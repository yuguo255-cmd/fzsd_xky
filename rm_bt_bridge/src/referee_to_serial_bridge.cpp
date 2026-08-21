#include <rclcpp/rclcpp.hpp>

#include <rm_referee_interfaces/msg/robot_status.hpp>
#include <rm_referee_interfaces/msg/game_status.hpp>
#include <rm_interfaces/msg/serial_receive_data.hpp>

// 把 PB 仿真裁判的 RobotStatus 转成 COD 行为树能读的 SerialReceiveData。
// 仿真里没有下位机串口，这个节点顶替 cod_serial 充当数据源。
class RefereeToSerialBridge : public rclcpp::Node
{
public:
  RefereeToSerialBridge() : Node("referee_to_serial_bridge")
  {
    this->declare_parameter<bool>("zone_status_default", false);
    this->declare_parameter<bool>("self_status_default", false);
    this->declare_parameter<bool>("is_defence_default", false);
    this->declare_parameter<bool>("is_attack_default", false);
    this->declare_parameter<bool>("is_recover_default", false);

    zone_status_ = this->get_parameter("zone_status_default").as_bool();
    self_status_ = this->get_parameter("self_status_default").as_bool();
    is_defence_ = this->get_parameter("is_defence_default").as_bool();
    is_attack_ = this->get_parameter("is_attack_default").as_bool();
    is_recover_ = this->get_parameter("is_recover_default").as_bool();

    robot_status_sub_ = this->create_subscription<rm_referee_interfaces::msg::RobotStatus>(
      "/referee/robot_status", 10,
      std::bind(&RefereeToSerialBridge::robot_status_callback, this, std::placeholders::_1));

    game_status_sub_ = this->create_subscription<rm_referee_interfaces::msg::GameStatus>(
      "/referee/game_status", 10,
      std::bind(&RefereeToSerialBridge::game_status_callback, this, std::placeholders::_1));

    serial_pub_ = this->create_publisher<rm_interfaces::msg::SerialReceiveData>(
      "/SerialReceiveData", 10);

    // 周期发布，即使裁判系统 1Hz 较慢也能维持行为树侧的心跳
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(200),
      std::bind(&RefereeToSerialBridge::publish, this));

    RCLCPP_INFO(this->get_logger(), "referee -> SerialReceiveData 桥接启动");
  }

private:
  void robot_status_callback(const rm_referee_interfaces::msg::RobotStatus::SharedPtr msg)
  {
    last_robot_status_ = *msg;
    have_robot_status_ = true;
  }

  void game_status_callback(const rm_referee_interfaces::msg::GameStatus::SharedPtr msg)
  {
    last_game_status_ = *msg;
    have_game_status_ = true;
  }

  void publish()
  {
    if (!have_robot_status_) {
      return;  // 还没收到裁判状态，先不发
    }

    auto out = rm_interfaces::msg::SerialReceiveData();
    out.judge_system_data.hp = static_cast<float>(last_robot_status_.remain_hp);
    out.judge_system_data.zone_status = zone_status_;
    out.judge_system_data.self_status = self_status_;
    out.judge_system_data.is_defence = is_defence_;
    out.judge_system_data.is_attack = is_attack_;
    out.judge_system_data.is_recover = is_recover_;
    serial_pub_->publish(out);
  }

  bool have_robot_status_ = false;
  bool have_game_status_ = false;
  rm_referee_interfaces::msg::RobotStatus last_robot_status_;
  rm_referee_interfaces::msg::GameStatus last_game_status_;

  bool zone_status_, self_status_, is_defence_, is_attack_, is_recover_;

  rclcpp::Subscription<rm_referee_interfaces::msg::RobotStatus>::SharedPtr robot_status_sub_;
  rclcpp::Subscription<rm_referee_interfaces::msg::GameStatus>::SharedPtr game_status_sub_;
  rclcpp::Publisher<rm_interfaces::msg::SerialReceiveData>::SharedPtr serial_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<RefereeToSerialBridge>());
  rclcpp::shutdown();
  return 0;
}
