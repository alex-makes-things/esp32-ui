#include <UIAssist.h>

//--------------------Cone STRUCT---------------------------------------------------------------//

/*!
    @brief Initialize a cone
    @param bisector        The angle that indicates the bisector of its aperture (Degrees)
    @param radius          How far the cone stretches out (Pixels)
    @param aperture        How wide the cone is (Degrees)
    @param aperture_step   How many degrees a calculation jumps over the loop of its aperture (Degrees)
    @param rad_step        How many pixels a calculation jumps over the loop of its radius (Pixels)
  */
Cone::Cone(unsigned int bisector, unsigned int radius, unsigned int aperture, unsigned int aperture_step, unsigned int rad_step)
    :bisector(bisector), radius(radius), aperture(aperture), aperture_step(aperture_step), rad_step(rad_step){}

/*!
    @brief Represent a 2D point with integer coordinates.
    @param    x   X coordinate of the point
    @param    y   Y coordinate of the point
*/
Point::Point(int posx, int posy) : x(posx), y(posy){}
//--------------------FOCUS STRUCT---------------------------------------------------------------//

Focus::Focus(std::string ele):focusedElementID(ele){}

/*!
    @brief Focus an object by its UUID
    @param ele The element UUID to focus
*/
void Focus::focus(std::string ele){
    previousElementID = focusedElementID;
    focusedElementID = ele;
  }

//This function needs to be called at the end of a cycle, which updates the previous focus id to the current one
void Focus::update(){
    previousElementID = focusedElementID;
    isFirstBoot = false;
  }

//!@return A boolean that when true, means that the focus has changed in the last cycle
bool Focus::hasChanged(){
    return (previousElementID != focusedElementID);
  }

/*!
  @return A boolean that when true, means that the passed object's identity is currently focused
  @param obj The UUID of the object in cause
*/
bool Focus::isFocusing(std::string obj){
    return (focusedElementID == obj);
  }

/*!
  @return A boolean that when true, means that the passed object's identity is currently focused
  @param obj The pointer of the object in cause
*/
bool Focus::isFocusing(UIElement* obj){
    return (focusedElementID == obj->getId());
  }

//--------------------ANIMATOR CLASS---------------------------------------------------------------//
Animator::Animator(float i, float f, unsigned int d){  //Basic constructor
    duration = d*1000;
    initial = i;
    final = f;
    progress = initial;
  }

void Animator::update(){
    if(reverse){
      //Magnetic attachment to initial value and setting isAtStart
        
        if (fabs(progress-final)<=0.0005f){
          progress = final;
        }
        isAtStart = (progress==final) ? true : false;
      }else{
        if (fabs(progress-initial)<=0.0005f){
          progress = initial;
        }
        isAtStart = (progress==initial) ? true : false;
      }

    //If the animation is active, the update logic runs
    if (enable){
      now = micros();
      elapsed = std::clamp(now-currentTime, static_cast<unsigned long long>(0), static_cast<unsigned long long>(duration));

      //Here I add anything that has to run when the animation is in its done state
      if(isDone){
        //Breathing feature
        if(breathing){
          if(looping){
            invert();
            bounce_done = false;
          } else if (!bounce_done){
            invert();
            isDone=false;
            bounce_done = true;
          }
        }
        //Restarts the animation if the looping condition is met
        if(looping){
          resetAnim();
        }
      }

      //Interpolation
      else{
        if (elapsed<duration){
          progress = std::clamp(Fmap((reverse) ? duration-elapsed : elapsed, 0, duration, initial, final), initial, final);

          } else {
            isDone = true;
            progress = (reverse) ? initial : final;
          }
      }
    }
  }


void Animator::setReverse(bool rev){
    reverse = rev;
    progress = (progress==initial&&reverse) ? final : progress;
  }

void Animator::stop(){
    update();
    enable = false;
  }

void Animator::switchState(){
    if (!enable){
      currentTime = micros()-elapsed;
    }else{update();}
    enable = !enable;
  }

void Animator::start(){
    enable = true;
    currentTime = micros()-elapsed;
  }
  
void Animator::defaultModifiers(){
    looping = false;
    reverse = false;
    breathing = false;
    bounce_done = false;
  }

void Animator::resetAnim(){
    progress = (reverse) ? final : initial;
    currentTime = micros();
    isDone = false;
    isAtStart = true;
  }

void Animator::invert(){ 
    reverse = !reverse;
    elapsed = duration-elapsed;
    currentTime = now-elapsed;
    now = currentTime;
  }

//--------------------UIElement CLASS---------------------------------------------------------------//

UIElement::UIElement(unsigned int w, unsigned int h, unsigned int posx, unsigned int posy, ElementType element, FocusStyle style) 
: type(element), m_width(w), m_height(h), m_position(Point(posx, posy)), focus_style(style){}

Point UIElement::centerToCornerPos(unsigned int x_pos, unsigned int y_pos, unsigned int w, unsigned int h){
      unsigned int new_x = static_cast<unsigned int>(static_cast<float>(x_pos)-(static_cast<float>(w)/2.0f));
      unsigned int new_y = static_cast<unsigned int>(static_cast<float>(y_pos)-(static_cast<float>(h)/2.0f));
      return Point(new_x, new_y);
    }

bool UIElement::isFocused(){
  return m_parent_ui->focus.focusedElementID==m_UUID;
}

void UIElement::drawOutline(){
  if (focus_style == FocusStyle::Outline && isFocused()) {

    Point rect_drawing_pos = centered ? centerToCornerPos(m_position.x, m_position.y, m_width, m_height) : m_position;
    rect_drawing_pos -= (outline.border_distance + 1);
    int16_t draw_width = m_width + outline.border_distance*2 +2;
    int16_t draw_height = m_height + outline.border_distance*2 +2;


    if(outline.radius != 0){
      int16_t draw_radius = outline.radius;
      for (int i = 0; i < outline.thickness; i++) {
        m_parent_ui->buffer->drawRoundRect(rect_drawing_pos.x, rect_drawing_pos.y, draw_width, draw_height, draw_radius, outline.color);
        if (!(i % 2)){
          int16_t temp_r = draw_radius + 1;
          m_parent_ui->buffer->drawCircleHelper(rect_drawing_pos.x + temp_r, rect_drawing_pos.y + temp_r, temp_r, 1, outline.color);
          m_parent_ui->buffer->drawCircleHelper(rect_drawing_pos.x + draw_width - temp_r - 1, rect_drawing_pos.y + temp_r, temp_r, 2, outline.color);
          m_parent_ui->buffer->drawCircleHelper(rect_drawing_pos.x + draw_width - temp_r - 1, rect_drawing_pos.y + draw_height - temp_r - 1, temp_r, 4, outline.color);
          m_parent_ui->buffer->drawCircleHelper(rect_drawing_pos.x + temp_r, rect_drawing_pos.y + draw_height - temp_r - 1, temp_r, 8, outline.color);
        }
        
        draw_width += 2;
        draw_height += 2; 
        rect_drawing_pos--;
        draw_radius++;
      }
    }
    else{

      for (int i = 0; i < outline.thickness; i++) {
        m_parent_ui->buffer->drawRect(rect_drawing_pos.x, rect_drawing_pos.y, draw_width, draw_height, outline.color);
        draw_width += 2;
        draw_height += 2;
        rect_drawing_pos--;
      }

    }
  }
}

void UIElement::render(){
  drawOutline();
}

//--------------------UIImage CLASS---------------------------------------------------------------//

void UIImage::render(){
  drawOutline();
  anim.update();
  
  if (draw){
    m_scale_fac = (m_overrideAnimationScaling) ? m_scale_fac : anim.getProgress();
    Image drawing_image = m_scale_fac != 1 ? scale(*m_body, m_scale_fac) : *m_body;
    Point drawing_pos = centered ? centerToCornerPos(m_position.x, m_position.y, drawing_image.width, drawing_image.height) : m_position;

    if(drawing_image.data.colorspace == PixelType::Mono){
      m_parent_ui->buffer->drawBitmap(drawing_pos.x, drawing_pos.y, drawing_image.data.mono, drawing_image.width, drawing_image.height, m_mono_color);
    }
    else{
      m_parent_ui->buffer->drawRGBBitmap(drawing_pos.x, drawing_pos.y, drawing_image.data.rgb565, drawing_image.width, drawing_image.height);
    }
  }
}

//--------------------AnimatedApp CLASS---------------------------------------------------------------//

void AnimatedApp::handleAppSelectionAnimation(){
  if (m_parent_ui->focus.hasChanged() || (m_parent_ui->focus.isFirstBoot && isFocused()))
  {
    if(isFocused()){  
    //If the element is being focused

      if(isAnimating() && anim.getDirection()){
        anim.invert();                         // If it's mid closing-animation, invert it
      }
      if(getActive()==m_unselected && anim.getStart())
      { //If it's showing the small img and hasn't started

        anim.start();
      }
    }
    else{ //Focus has changed but the element isn't focused
      if(getActive()==m_selected && anim.getDone() && !anim.getDirection())
      {
        anim.setFinal(m_ratio);
        m_showing = m_unselected;
        anim.resetAnim();
        anim.setReverse(true);
      }
      if(isAnimating() && !anim.getDirection()){
        anim.invert();                          // If it's mid opening-animation, invert it
      }
    }
  }
  else{
    if (getActive()==m_unselected && anim.getDone()){
      if (!anim.getDirection()){
        //Since we animated from a small image to a larger image, setting final to 1 automatically clamps it to 1
        anim.setFinal(1); 
        m_showing = m_selected;
      }
      else //This branch however, only runs on the falling part of the animation
      {
        //After the sum of all the conditions, we know that the animation has finished reversing to the smaller image
        anim.setFinal(m_ratio);
        anim.setReverse(false);  //Make sure the animation goes in the right direction when it is focused again
        anim.resetAnim();
        anim.stop();   
      }
    }
    
    if(!isFocused() && !isAnimating() && m_showing == m_selected){
      anim.setFinal(m_ratio);
      m_showing = m_unselected;
      anim.resetAnim();
      anim.setReverse(true);
    }
    if (isFocused() && anim.getStart() && !anim.getIsEnabled() && m_showing == m_unselected){
      anim.start();
    }
  }
}

void AnimatedApp::render(){
  switch(focus_style){
    case FocusStyle::Animation: handleAppSelectionAnimation();
    case FocusStyle::Outline: drawOutline();
  }
 
  anim.update();
  float scale_fac = anim.getProgress();
  if (draw){
    Image drawing_image = scale_fac != 1 ? scale(*m_showing, scale_fac) : *m_showing;
    Point drawing_pos = centered ? centerToCornerPos(m_position.x, m_position.y, drawing_image.width, drawing_image.height) : m_position;

    if(drawing_image.data.colorspace == PixelType::Mono){
      m_parent_ui->buffer->drawBitmap(drawing_pos.x, drawing_pos.y, drawing_image.data.mono, drawing_image.width, drawing_image.height, m_mono_color);
    }
    else{
      m_parent_ui->buffer->drawRGBBitmap(drawing_pos.x, drawing_pos.y, drawing_image.data.rgb565, drawing_image.width, drawing_image.height);
    }
  }
}


//--------------------Scene STRUCT---------------------------------------------------------------//

Scene::Scene(std::vector<UIElement*> elementGroup, UIElement& first_focus){
    for(int i = 0; i<elementGroup.size(); i++){
      elements.insert({elementGroup[i]->getId(),elementGroup[i]});
    }
    primaryElementID = first_focus.getId();
  }

void Scene::renderScene()
  {
    for (auto elem : elements)
    {
      elem.second->render();
    }
  }

UIElement* Scene::getElementByUUID(std::string UUID){
  return elements.at(UUID);
}

  //--------------------UI CLASS---------------------------------------------------------------//

  UI::UI(Scene& first_scene, GFXcanvas16& framebuffer)
  {
    focus = Focus(first_scene.primaryElementID);
    buffer = &framebuffer;
    addScene(&first_scene);
    focusScene(&first_scene);
  }

void UI::addScene(Scene* scene){
    scenes.push_back(scene);                 //KEEP IN MIND "REALLOCATES"
    for(auto elem : scene->elements){
      elem.second->setUiListener(this);
    }
  }

void UI::focusScene(Scene* scene){
    focus.focusedScene = scene;
    focus.focusedElementID = focus.focusedScene->primaryElementID;
  }



/// @brief Focus the closest object in any direction
/// @param direction The direction in counter clockwise degrees, with its origin being the center of the currently focused element (Right is 0)
/// @param alg The focusing algorithm that you want to use (FocusingAlgorithm::Linear, FocusingAlgorithm::Cone)
void UI::focusDirection(unsigned int direction, FocusingAlgorithm alg){
  if(isFocusingFree()){
    m_focusing_busy = true;
    UIElement* next_element = UiUtils::SignedDistance(direction, focusingSettings, alg, focus.focusedScene, focus.focusedScene->getElementByUUID(focus.focusedElementID));
    if(next_element){
        focus.focus(next_element->getId());
        focus.focusedElementID = next_element->getId();
    }
  }
}

void UI::update(){
  focus.update();
  m_focusing_busy = false;
}

//--------------------UiUtils NAMESPACE---------------------------------------------------------------//

const float UiUtils::degToRadCoefficient= 0.01745329251;

bool UiUtils::isPointInElement(Point point, UIElement* element){
    Point element_pos = element->getPos();
    if ((point.x >= element_pos.x && point.x <= element_pos.x + element->getWidth()) && (point.y >= element_pos.y && point.y <= element_pos.y + element->getHeight())){
      return true;
    }
    return false;
  }

Point UiUtils::centerPos(int x_pos, int y_pos, unsigned int w, unsigned int h){
      int new_x = floor((float)x_pos+((float)w/2));
      int new_y = floor((float)y_pos+((float)h/2));
      return Point(new_x, new_y);
  }

UIElement* UiUtils::SignedDistance(unsigned int direction, FocusingSettings settings, FocusingAlgorithm alg, Scene* scene, UIElement* focused){

    Point center_point = (focused->centered) ? focused->getPos() 
                                                   : UiUtils::centerPos(focused->getPos().x, focused->getPos().y, focused->getWidth(), focused->getHeight());
    Scene temporaryScene = *scene;
    temporaryScene.elements.erase(focused->getId());
    Point new_point = center_point;
    if (alg == FocusingAlgorithm::Linear)
    {
      Ray ray{settings.max_distance, 1, direction};
      switch (settings.accuracy)
      {
      case Quality::Low:
        ray.step = 4;
        break;
      case Quality::Medium:
        ray.step = 2;
        break;
      case Quality::High:
        ray.step = 1;
        break;
      }
      return findElementInRay(focused, scene, ray);
    }

    else  //IF THE METHOD IS CONE
    {
      Cone cone(direction, settings.max_distance, 90, 2, 6);
      switch (settings.accuracy){
        case Quality::Low:
          cone.aperture_step = 3;
          cone.rad_step = 8;
          break;
        case Quality::Medium:
          cone.aperture_step = 2;
          cone.rad_step = 6;
          break;
        case Quality::High:
          cone.aperture_step = 1;
          cone.rad_step = 2;
          break;
        }
      return findElementInCone(focused, scene, cone);
    }

    return nullptr;
}

Point UiUtils::polarToCartesian(float radius, float angle){
  int x = radius * cos(-angle * degToRadCoefficient);
  int y = radius * sin(-angle * degToRadCoefficient);
  return Point(x, y);
}

std::set<Point> UiUtils::computeConePoints(Point vertex, Cone cone){
  std::set<Point> buffer;

  int half_aperture = cone.aperture/2;
  int starting_angle = cone.bisector - half_aperture;
  int end_angle = cone.bisector + half_aperture;

  for(int i = starting_angle; i<end_angle; i+=cone.aperture_step){
    for(int b = 0; b<cone.radius; b+=cone.rad_step){
      Point tempPoint = polarToCartesian(b, i);
      tempPoint.x += vertex.x;
      tempPoint.y += vertex.y;
      buffer.emplace(tempPoint);
    }
  }
  return buffer;
}

UIElement* UiUtils::findElementInCone(UIElement *focused, Scene *currentScene, Cone cone){
  int half_aperture = cone.aperture / 2;
  int starting_angle = cone.bisector - half_aperture;
  int end_angle = cone.bisector + half_aperture;
  Scene tempScene = *currentScene;
  tempScene.elements.erase(focused->getId());

  for (int i = starting_angle; i < end_angle; i += cone.aperture_step)
  {
    for (int b = 0; b < cone.radius; b += cone.rad_step)
    {
      Point tempPoint = polarToCartesian(b, i);
      Point centerPoint = focused->getPos();
      tempPoint.x += centerPoint.x;
      tempPoint.y += centerPoint.y;
      for(auto elem : tempScene.elements){
        if (isPointInElement(tempPoint, elem.second)){
          return elem.second;
        }
      }
    }
  }
  return nullptr;
}

UIElement* UiUtils::findElementInRay(UIElement *focused, Scene *currentScene, Ray ray){
  Scene tempScene = *currentScene;
  tempScene.elements.erase(focused->getId());

  for(int i = 0; i<ray.ray_length; i+=ray.step){
    Point tempPoint = polarToCartesian(i, ray.direction);
    Point centerPoint = focused->getPos();
    tempPoint.x += centerPoint.x;
    tempPoint.y += centerPoint.y;
    for(auto elem : tempScene.elements){
      if (isPointInElement(tempPoint, elem.second)){
        return elem.second;
      }
    }
  }
  return nullptr;
}