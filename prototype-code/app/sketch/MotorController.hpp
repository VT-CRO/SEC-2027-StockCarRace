#ifndef MOTOR_CONTROLLER_H_
#define MOTOR_CONTROLLER_H_

/*
 * MotorController.hpp provides a single abstract class to generalize
 * controlling a motor.
 *
 * Individual motor motor controller implementations may implement it in order
 * to decouple the motor controller IO from higher level logic in code.
 *
 * It also provides the MotorDirection enum, which implementations may use to
 * define a direction.
 */

enum MotorDirection {
  CLOCKWISE_POSITIVE,
  COUNTER_CLOCKWISE_POSITIVE,
};

/*
 * A MotorController is an interface for controlling a motor by commanding it to
 * run at a certain velocity.
 */
class MotorController {
public:
  /*
   * runOpenLoop runs the motor at a given velocity, from -255 (full speed
   * backward) to 255 (full speed forward).
   */
  virtual void runOpenLoop(int velocity) = 0;

  /*
   * Stops the motors. This may behave differently than runOpenLoop and is
   * preferable for stopping the robot.
   */
  virtual void stop();

  virtual ~MotorController() = default;
};

#endif