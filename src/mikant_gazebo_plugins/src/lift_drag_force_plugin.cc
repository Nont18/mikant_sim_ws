#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/common/common.hh>
#include <gazebo/common/Assert.hh>
#include <gazebo/common/Events.hh>
#include <ignition/math/Vector3.hh>
#include <gazebo_ros/node.hpp>

#include <rclcpp/rclcpp.hpp>
#include "std_msgs/msg/float64_multi_array.hpp"
#include "mikant_gazebo_plugins/lift_drag_force_plugin.hh"
#include <string>

namespace lift_drag_force_plugin

  LiftDragForcePlugin::LiftDragForcePlugin()
    : fluid_density(1.225),
      center_of_pressure(0, 0, 0)
  {   
  }

  void LiftDragForcePlugin::Load(gazebo::physics::ModelPtr _model, sdf::ElementPtr _sdf)
  {
    // Store the pointer to the model
    this->model = _model;
    this->world = this->model->GetWorld();
    this->link_name = _sdf->GetElement("link_name")->Get<std::string>();
    this->link = model->GetLink(link_name);
    this->link_id = link->GetId();

    if(_sdf->HasElement("fluid_density"))
    {
      this->fluid_density = _sdf->Get<double>("fluid_density");
    }
    if(_sdf->HasElement("foil_area"))
    {
      this->foil_area = _sdf->Get<double>("foil_area");
    }
    if(_sdf->HasElement("cla"))
    {
      this->cla = _sdf->Get<double>("cla");
    }
    if(_sdf->HasElement("cda0"))
    {
      this->cda0 = _sdf->Get<double>("cda0");
    }
    if(_sdf->HasElement("cda"))
    {
      this->cda = _sdf->Get<double>("cda");
    }
    if(_sdf->HasElement("init_angle_of_attack"))
    {
      this->init_angle_of_attack = _sdf->Get<double>("init_angle_of_attack");
    }
    if(_sdf->HasElement("span"))
    {
      this->span = _sdf->Get<double>("span");
    }

    this->init_ros_node(gazebo_ros::Node::Get(_sdf))
    
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
      this->top_level = 0.163334;
      this->bottom_level = 0.036666;
    }
    else if(this->dif_level < 0.2 && this->dif_level >= 0.036666)
    {
      this->bottom_level = 0.036666;
      this->top_level = this->dif_level - this->bottom_level;
    }
    else if(this->dif_level < 0.036666 && this->dif_level > 0)
    {
      this->bottom_level = this->dif_level;
      this->top_level = 0;
    }
    else
    {
      this->bottom_level = 0;
      this->top_level = 0;
    }
    
    this->displaced_volume = (0.1895489009 * this->bottom_level) + (0.218 * this->top_level);
    
    if(this->displaced_volume > this->link_volume)
    {
      this->displaced_volume = this->link_volume;
    } 

    if(this->displaced_volume > 1e-6)
    {
    
    double total_link_mass = 0.0;
    ignition::math::Vector3d model_cg_position = ignition::math::Vector3d(0, 0, 0);
    for(unsigned int i = 0; i < model->GetChildCount(); i++)
    {
      gazebo::physics::BasePtr temp_base = model->GetChild(i);
      if(temp_base->HasType(gazebo::physics::Base::LINK))
      {
        gazebo::physics::LinkPtr temp_link = model->GetLink(temp_base->GetName());
        double temp_link_mass = temp_link->GetInertial()->Mass();
        model_cg_position.operator+=(temp_link->WorldCoGPose().Pos() * temp_link_mass);
        total_link_mass = total_link_mass + temp_link_mass;
      }
    }
    model_cg_position = model_cg_position/total_link_mass;


      ignition::math::Vector3d buoyancy_force = -this->fluid_density * this->displaced_volume * model->GetWorld()->Gravity();
    
      ignition::math::Vector3d link_Vel = link->WorldLinearVel();

      float partialMass = total_link_mass * displaced_volume / link_volume;
      
      //find velocity different
      ignition::math::Vector3d rel_vel = ignition::math::Vector3d(0, 0, 0) - link_Vel; 

      ignition::math::Vector3d dragforce = linear_drag * partialMass * rel_vel; //use linear drag = 1 

      buoyancy_force += dragforce;

    if(buoyancy_force.Z() < 0)
    {
      buoyancy_force.Z() = 0;
    }


    link->AddForceAtWorldPosition(buoyancy_force, model_cg_position);
    
    }
    
  }
    
  // Register this plugin with the simulator
  GZ_REGISTER_MODEL_PLUGIN(BuoyancyForcePlugin)
}