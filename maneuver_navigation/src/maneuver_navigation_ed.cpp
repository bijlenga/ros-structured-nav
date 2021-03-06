

#include "maneuver_navigation_ed.h"



// std::vector<geometry_msgs::PoseStamped> global_path;
bool goal_received = false;
geometry_msgs::PoseStamped goal;
void goalCallback(const geometry_msgs::PoseStamped::ConstPtr& goal_msg)
{
    ROS_INFO("new goal received");
    goal = *goal_msg;
    goal_received = true;
}

bool reinit_planner_withload = false;
bool reinit_planner_noload = false;
void loadAttachedCallback(const std_msgs::Bool::ConstPtr& load_attached_msg)
{   
    ROS_INFO("Reinit PLanner");
    if( load_attached_msg->data == true )
        reinit_planner_withload = true;
    else
        reinit_planner_noload = true;
}

ManeuverNavigationED::ManeuverNavigationED()
{
}

// ----------------------------------------------------------------------------------------------------

ManeuverNavigationED::~ManeuverNavigationED()
{
}

void ManeuverNavigationED::initialize(ed::InitData& init)
{
    //ros::init(argc, argv, "route_navigation");
    ros::NodeHandle n("~");    
    
   // double prediction_feasibility_check_rate, prediction_feasibility_check_period, prediction_feasibility_check_cycle_time = 0.0;
   // double local_navigation_rate, local_navigation_period; 
    
   prediction_feasibility_check_cycle_time_ = 0.0;

    n.param<double>("prediction_feasibility_check_rate", prediction_feasibility_check_rate_, 2.0);
    n.param<double>("local_navigation_rate", local_navigation_rate_, 10.0); // local_navigation_rate>prediction_feasibility_check_rate
    ros::Rate rate(local_navigation_rate_);
    prediction_feasibility_check_period_ = 1.0/prediction_feasibility_check_rate_;
    local_navigation_period_ = 1.0/local_navigation_rate_;
    
    ros::Subscriber goal_cmd_sub = n.subscribe<geometry_msgs::PoseStamped>("/route_navigation/goal", 10, goalCallback);
    ros::Subscriber load_attached_sub = n.subscribe<std_msgs::Bool>("/route_navigation/load_attached", 10, loadAttachedCallback);
        
//     /move_base_simple/goal
    tf::TransformListener tf( ros::Duration(10) );
    mn::ManeuverNavigation maneuver_navigator(tf,n);
    maneuver_navigator_ = maneuver_navigator;
    maneuver_navigator_.init();


    nav_msgs::Path path_msg;

    ROS_INFO("Wait for goal");
    
}
   // while(n.ok())
 //   {
 ManeuverNavigationED::process(const ed::WorldModel& world, ed::UpdateRequest& req)
 {
        prediction_feasibility_check_cycle_time_ += local_navigation_period_;
        // Execute local navigation
        maneuver_navigator_.callLocalNavigationStateMachine();
        
        if (goal_received)
        {
            goal_received = false;             
        
            maneuver_navigator_.gotoGoal(goal);
            prediction_feasibility_check_cycle_time_ = prediction_feasibility_check_period_;
        }         
        
        // Execute route navigation
        if( prediction_feasibility_check_cycle_time_ > prediction_feasibility_check_period_)
        { 
            prediction_feasibility_check_cycle_time_ = 0.0;
            maneuver_navigator_.callManeuverNavigationStateMachine();
        }
        
        
       if(reinit_planner_withload)
        {
            reinit_planner_withload = false;
            // TODO:: Read load footprint from file and do it asynchronously for not interrupting the therad
            geometry_msgs::Polygon newfootprint;
            geometry_msgs::Point32 point_footprint;
            point_footprint.x = -0.1; point_footprint.y =  0.36; point_footprint.z = 0.0;
            newfootprint.points.push_back(point_footprint);
            point_footprint.x =  1.3; point_footprint.y =  0.36; point_footprint.z = 0.0;
            newfootprint.points.push_back(point_footprint);
            point_footprint.x =  1.3; point_footprint.y = -0.36; point_footprint.z = 0.0;
            newfootprint.points.push_back(point_footprint);
            point_footprint.x = -0.1; point_footprint.y = -0.36; point_footprint.z = 0.0;
            newfootprint.points.push_back(point_footprint);
            system("rosparam load ~/ropod-project-software/catkin_workspace/src/applications/ropod_navigation_test/config/parameters/teb_local_planner_params.yaml maneuver_navigation");

            maneuver_navigator_.reinitPlanner(newfootprint);  // Dynamic reconfigurationdid not work so we had to do it in two ways. The localcostmap
            // was updated directly with functins, and the tebplanner by setting firts the parameters and then reloading the planner

        }           
        
        if(reinit_planner_noload)
        {
            reinit_planner_noload = false;
            // TODO:: Read load footprint from file and do it asynchronously for not interrupting the therad
            geometry_msgs::Polygon newfootprint;
            geometry_msgs::Point32 point_footprint;
            point_footprint.x = -0.36; point_footprint.y =  0.36; point_footprint.z = 0.0;
            newfootprint.points.push_back(point_footprint);
            point_footprint.x =  0.36; point_footprint.y =  0.36; point_footprint.z = 0.0;
            newfootprint.points.push_back(point_footprint);
            point_footprint.x =  0.36; point_footprint.y = -0.36; point_footprint.z = 0.0;
            newfootprint.points.push_back(point_footprint);
            point_footprint.x = -0.36; point_footprint.y = -0.36; point_footprint.z = 0.0;
            newfootprint.points.push_back(point_footprint);
            system("rosparam load ~/ropod-project-software/catkin_workspace/src/applications/ropod_navigation_test/config/parameters/teb_local_planner_params_ropod.yaml maneuver_navigation");
            maneuver_navigator_.reinitPlanner(newfootprint);  // Dynamic reconfigurationdid not work so we had to do it in two ways. The localcostmap
            // was updated directly with functins, and the tebplanner by setting firts the parameters and then reloading the planner            
        }        

       

      //  ros::spinOnce();
     //   rate.sleep();
//         if(rate.cycleTime() > ros::Duration(local_navigation_period_) )
//             ROS_WARN("Control loop missed its desired rate of %.4fHz... the loop actually took %.4f seconds", local_navigation_rate, rate.cycleTime().toSec());

    }



}

ED_REGISTER_PLUGIN(ManeuverNavigationED)
