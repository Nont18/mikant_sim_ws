#include <functional>
#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/common/common.hh>
#include <gazebo/common/Assert.hh>
#include <gazebo/common/Events.hh>
#include <ignition/math/Vector3.hh>

#include "mikant_gazebo_plugins/buoyancy_force_plugin.hh"
#include <string>

namespace buoyancy_force_plugin
{

  BuoyancyForcePlugin::BuoyancyForcePlugin()
    : fluid_density(998),
      fluid_level(0),
      link_pose(0, 0, 0, 0, 0, 0)
  {   
  }

  void BuoyancyForcePlugin::Load(gazebo::physics::ModelPtr _model, sdf::ElementPtr _sdf)
  {
    // Store the pointer to the model
    this->model = _model;
    this->world = this->model->GetWorld();
    this->link_name = _sdf->GetElement("link_name")->Get<std::string>();
    this->link = model->GetLink(link_name);
    this->link_id = link->GetId();

    if(_sdf->HasElement("link_volume"))
    {
      this->link_volume = _sdf->Get<double>("link_volume");
    }
    if(_sdf->HasElement("fluid_density"))
    {
      this->fluid_density = _sdf->Get<double>("fluid_density");
    }
    if(_sdf->HasElement("fluid_level"))
    {
      this->fluid_level = _sdf->Get<double>("fluid_level");
    }

    
    // Listen to the update event. This event is broadcast every
    // simulation iteration.
    this->updateConnection = gazebo::event::Events::ConnectWorldUpdateBegin(std::bind(&BuoyancyForcePlugin::OnUpdate, this));
  }

  // Called by the world update start event
  void BuoyancyForcePlugin::OnUpdate()
  {    
    this->link_pose = link->WorldPose();

    this->level = this->link_pose.Z();
    
    this->dif_level = this->fluid_level - this->level;


    // Check where is the water level at
    if(this->dif_level >= 0.2)
    {
      this->displaced_volume = this->link_volume;
    }
    else if(this->dif_level < 0.2 && this->dif_level >= 0.036666)
    {
      this->bottom_level = 0.036666;
      this->top_level = this->dif_level - this->bottom_level;
    }
    else if(this->dif_level < 0.036666)
    {
      this->bottom_level = 0.036666 - this->dif_level;
      this->top_level = 0;
    }
    else
    {
      this->displaced_volume = 0;
    }
    
    this->displaced_volume = (0.1895489009 * this->bottom_level) + (0.218 * this->top_level);

    if(this->displaced_volume > 1e-6)
    {
      ignition::math::Vector3d buoyancy_force = this->fluid_density * this->displaced_volume * model->GetWorld()->Gravity();

    if(buoyancy_force.Z() < 0)
    {
      buoyancy_force.Z() = 0;
    }

    link->AddRelativeForce(buoyancy_force);
    
    }
    
  }
    
  // Register this plugin with the simulator
  GZ_REGISTER_MODEL_PLUGIN(BuoyancyForcePlugin)
}