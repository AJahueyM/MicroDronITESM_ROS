/**
 * Created by alberto on 27/02/19. 
 */

#include "Drone.h"

Drone::Drone() {
    angularVelocityPub = nodeHandle.advertise<mav_msgs::Actuators>("/ardrone/command/motor_speed", 1000);
    imuSub = nodeHandle.subscribe("/ardrone/imu", 1000, &Drone::onImuMessageReceived , this);
    irSub = nodeHandle.subscribe("/ardrone/ir", 1000, &Drone::onIRReceived, this);
    SimplePIDConfig yawConfig, pitchConfig, rollConfig, heightConfig;

    yawConfig.p = 0.7;
    yawConfig.i = 0.0;
    yawConfig.d = 0.8;
    yawConfig.clampedOutput = true;
    yawConfig.max = 0.2;
    yawConfig.min = -0.2;

    pitchConfig.p = 0.2;
    pitchConfig.i = 0.0;
    pitchConfig.d = 0.06;

    rollConfig.p = 0.2;
    rollConfig.i = 0.0;
    rollConfig.d = 0.06;

    heightConfig.p = 1.2;
    heightConfig.i = 0.0;
    heightConfig.d = 0.5;
    heightConfig.feedForward = 0.788;

    yawPID.setConfig(yawConfig);
    pitchPID.setConfig(pitchConfig);
    rollPID.setConfig(rollConfig);
    heightPID.setConfig(heightConfig);
}

void Drone::update() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    heartbeatTime = std::chrono::duration<double>(currentTime - lastHeartbeatTime).count();

    if(heartbeatTime > 1){
        yawPID.setSetpoint(0);
        pitchPID.setSetpoint(0);
        rollPID.setSetpoint(0);
        ROS_ERROR("Heartbeat not reset! Be sure to call Drone::sendHeartBeat at least once per second.");
    }

    mav_msgs::Actuators output;
    output.angular_velocities.clear();

    if(!manualOutput){
        double yawRate = yawPID.update(yaw);
        double pitchRate = pitchPID.update(pitch);
        double rollRate = rollPID.update(roll);
        double thrust = heightPID.update(height);

        motorOutputs[0] = (thrust - pitchRate - rollRate - yawRate) * k;
        motorOutputs[1] = (thrust + pitchRate + rollRate - yawRate) * k;
        motorOutputs[2] = (thrust - pitchRate + rollRate + yawRate) * k;
        motorOutputs[3] = (thrust + pitchRate - rollRate + yawRate) * k;


        motorOutputs[0] = motorOutputs[0] < 0 ? 0 : motorOutputs[0];
        motorOutputs[1] = motorOutputs[1] < 0 ? 0 : motorOutputs[1];
        motorOutputs[2] = motorOutputs[2] < 0 ? 0 : motorOutputs[2];
        motorOutputs[3] = motorOutputs[3] < 0 ? 0 : motorOutputs[3];
    }

    double topRightAngularSpeed = motorOutputs[0];
    double bottomLeftAngularSpeed = motorOutputs[1];
    double topLeftAngularSpeed = motorOutputs[2];
    double bottomRightAngularSpeed = motorOutputs[3];

    output.angular_velocities.emplace_back(topRightAngularSpeed);
    output.angular_velocities.emplace_back(bottomLeftAngularSpeed);
    output.angular_velocities.emplace_back(topLeftAngularSpeed);
    output.angular_velocities.emplace_back(bottomRightAngularSpeed);
    angularVelocityPub.publish(output);
}

const SimplePID &Drone::getPitchPID() const{
    return pitchPID;
}

const SimplePID &Drone::getRollPID() const{
    return rollPID;
}

const SimplePID &Drone::getYawPID() const{
    return yawPID;
}

const SimplePID &Drone::getHeightPID() const{
    return heightPID;
}

void Drone::setPitchPID(SimplePID pitchPID){
    this->pitchPID = pitchPID;
}

void Drone::setRollPID(SimplePID rollPID){
    this->rollPID = rollPID;
}

void Drone::setYawPID(SimplePID yawPID){
    this->yawPID = yawPID;
}

void Drone::setHeightPID(SimplePID heightPID){
    this->heightPID = heightPID;
}

void Drone::sendHeartBeat(){
    lastHeartbeatTime = std::chrono::high_resolution_clock::now();
}

float Drone::getPitch() const{
    return pitch;
}

float Drone::getRoll() const{
    return roll;
}

float Drone::getYaw() const{
    return yaw;
}

float Drone::getHeight() const{
    return height;
}

int Drone::getMode() const{
    return manualOutput;
}

float Drone::getK() const{
    return k;
}

float Drone::getMotorOutputTR() const{
    return motorOutputs[0];
}

float Drone::getMotorOutputBL() const{
    return motorOutputs[1];
}

float Drone::getMotorOutputTL() const{
    return motorOutputs[2];
}

float Drone::getMotorOutputBR() const{
    return motorOutputs[3];
}

void Drone::setAllMotorOutput(float output){
    setAllMotorOutput(output, output, output, output);
}

void Drone::setAllMotorOutput(float topRight, float bottomLeft, float topLeft, float bottomRight){
    manualOutput = true;
    motorOutputs[0] = topRight;
    motorOutputs[1] = bottomLeft;
    motorOutputs[2] = topLeft;
    motorOutputs[3] = bottomRight;
}

void Drone::setSetpoints(float pitchSetpoint, float rollSetpoint, float yawSetpoint, float heightSetpoint){
    manualOutput = false;
    pitchPID.setSetpoint(pitchSetpoint);
    rollPID.setSetpoint(rollSetpoint);
    yawPID.setSetpoint(yawSetpoint);
    heightPID.setSetpoint(heightSetpoint);
}

void Drone::setK(float newK){
    this->k = newK;
}

void Drone::emergencyStop(){
    setAllMotorOutput(0);
}

bool Drone::isConnected() const{
    return true;
}

float Drone::getHeartbeatTime() const{
    return heartbeatTime;
}

void Drone::onImuMessageReceived(const sensor_msgs::Imu::ConstPtr& msg){
    auto q = msg->orientation;
    // roll (x-axis rotation)
    double sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
    double cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
    roll = std::atan2(sinr_cosp, cosr_cosp);

    // pitch (y-axis rotation)
    double sinp = 2 * (q.w * q.y - q.z * q.x);
    if (std::abs(sinp) >= 1)
        pitch = std::copysign(M_PI / 2, sinp); // use 90 degrees if out of range
    else
        pitch = std::asin(sinp);

    // yaw (z-axis rotation)
    double siny_cosp = 2 * (q.w * q.z + q.x * q.y);
    double cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
    yaw = std::atan2(siny_cosp, cosy_cosp);
}

void Drone::onIRReceived(const sensor_msgs::Range::ConstPtr& msg){
    height = msg->range * (std::cos(roll) * std::cos(pitch));
}

Drone::~Drone(){
    mav_msgs::Actuators output;
    output.angular_velocities.clear();

    double topRightAngularSpeed = motorOutputs[0];
    double bottomLeftAngularSpeed = motorOutputs[1];
    double topLeftAngularSpeed = motorOutputs[2];
    double bottomRightAngularSpeed = motorOutputs[3];

    output.angular_velocities.emplace_back(topRightAngularSpeed);
    output.angular_velocities.emplace_back(bottomLeftAngularSpeed);
    output.angular_velocities.emplace_back(topLeftAngularSpeed);
    output.angular_velocities.emplace_back(bottomRightAngularSpeed);
    angularVelocityPub.publish(output);
}
