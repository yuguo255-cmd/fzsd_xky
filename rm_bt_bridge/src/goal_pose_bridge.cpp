#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>

#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav2_msgs/action/navigate_to_pose.hpp>

#include <cmath>

// COD 行为树用 PubNav2Goal 向 /goal_pose 话题发目标，但标准 Nav2 只提供
// /navigate_to_pose action，不订阅 /goal_pose。这个节点把话题目标转成 action 目标。
//
// 两个关键点：
// 1) 按「位置」去重：同一目标点(阈值内)直接忽略，避免行为树每 500ms 重发一次
//    反复打断 Nav2 规划。
// 2) 换目标时只取消「当前那一个」goal 句柄，绝不用 async_cancel_all_goals()——
//    因为它是异步的，晚几毫秒执行时会误杀刚下发的新目标，造成取消/重发死循环。
class GoalPoseBridge : public rclcpp::Node
{
public:
  using NavigateToPose = nav2_msgs::action::NavigateToPose;
  using GoalHandle = rclcpp_action::ClientGoalHandle<NavigateToPose>;

  GoalPoseBridge() : Node("goal_pose_bridge")
  {
    this->declare_parameter<double>("goal_change_tolerance", 0.3);

    goal_pose_sub_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(
      "/goal_pose", 10,
      std::bind(&GoalPoseBridge::goal_callback, this, std::placeholders::_1));

    nav_client_ = rclcpp_action::create_client<NavigateToPose>(this, "/navigate_to_pose");

    RCLCPP_INFO(this->get_logger(), "/goal_pose -> navigate_to_pose 桥接启动");
  }

private:
  static double dist2d(
    const geometry_msgs::msg::PoseStamped & a,
    const geometry_msgs::msg::PoseStamped & b)
  {
    double dx = a.pose.position.x - b.pose.position.x;
    double dy = a.pose.position.y - b.pose.position.y;
    return std::sqrt(dx * dx + dy * dy);
  }

  void goal_callback(const geometry_msgs::msg::PoseStamped::SharedPtr msg)
  {
    // 1) 位置去重：与上次已下发目标足够接近，直接忽略
    if (have_last_goal_) {
      double tol = this->get_parameter("goal_change_tolerance").as_double();
      if (dist2d(last_goal_, *msg) < tol) {
        return;
      }
    }

    if (!nav_client_->action_server_is_ready()) {
      RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 3000,
        "Nav2 /navigate_to_pose 动作服务器未就绪");
      return;
    }

    // 2) 换目标：只取消当前那一个 goal，并立刻释放句柄
    if (current_goal_handle_) {
      nav_client_->async_cancel_goal(current_goal_handle_);
      current_goal_handle_.reset();
    }

    // 3) 下发新目标
    auto goal = NavigateToPose::Goal();
    goal.pose = *msg;
    if (goal.pose.header.frame_id.empty()) {
      goal.pose.header.frame_id = "map";
    }
    goal.pose.header.stamp = this->now();

    uint64_t my_id = ++goal_counter_;
    auto options = rclcpp_action::Client<NavigateToPose>::SendGoalOptions();
    options.goal_response_callback =
      [this, my_id](GoalHandle::SharedPtr handle) {
        if (my_id != goal_counter_) {
          return;  // 已被更新的目标取代
        }
        if (handle) {
          current_goal_handle_ = handle;
          // 只有服务器接受，才记作「上次目标」参与去重
          have_last_goal_ = true;
        } else {
          // 被拒绝：清掉去重标记，下次同一目标会重试
          have_last_goal_ = false;
        }
      };
    options.result_callback =
      [this, my_id](const GoalHandle::WrappedResult & result) {
        if (my_id == goal_counter_) {
          current_goal_handle_.reset();
          // 成功：机器人已到目标，保持去重（不再重复发同一目标）
          // 失败/取消：清掉去重标记，行为树下次再发同一目标时重试，
          // 避免「导航失败一次后就永远干愣着」
          if (result.code != rclcpp_action::ResultCode::SUCCEEDED) {
            have_last_goal_ = false;
          }
        }
      };

    nav_client_->async_send_goal(goal, options);
    last_goal_ = *msg;

    RCLCPP_INFO(this->get_logger(), "下发导航目标 [%.2f, %.2f]",
                goal.pose.pose.position.x, goal.pose.pose.position.y);
  }

  bool have_last_goal_ = false;
  uint64_t goal_counter_ = 0;
  geometry_msgs::msg::PoseStamped last_goal_;
  GoalHandle::SharedPtr current_goal_handle_;

  rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr goal_pose_sub_;
  rclcpp_action::Client<NavigateToPose>::SharedPtr nav_client_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<GoalPoseBridge>());
  rclcpp::shutdown();
  return 0;
}
