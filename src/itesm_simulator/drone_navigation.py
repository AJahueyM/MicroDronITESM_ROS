import rospy 
from geometry_msgs.msg import PoseStamped
from sensor_msgs.msg import Imu, NavSatFix
from std_msgs.msg import Float32, String
from pyquaternion import Quaternion
from tf.transformations import quaternion_from_euler
from readers    import *
from enums      import *
import time
import math

class Follower:
    def __init__(self,x,y,z,topic,heightFactor = 1):
        self.current_x = x
        self.current_y = y
        self.current_z = z
        self.heightFactor = heightFactor
        self.current_yaw = quaternion_from_euler(0, 0, 0)
        self.position_target_pub = rospy.Publisher(f'/{topic}/command/pose', PoseStamped, queue_size=10)

    def move(self, x, y, z, BODY_OFFSET_ENU=True):
        self.position_target_pub.publish(self.set_pose(x, y, z, BODY_OFFSET_ENU))

    def turn(self, yaw_degree):
        self.position_target_pub.publish(self.set_orientation(yaw_degree))

    # return to home position with defined height
    def return_home(self, height):
        self.position_target_pub.publish(self.set_pose(0, 0, height, False))


    def set_pose(self, x=0, y=0, z=2, bodyFlu = True):
        pose = PoseStamped()
        pose.header.stamp = rospy.Time.now()

        # ROS uses ENU internally, so we will stick to this convention
        if bodyFlu:
            # pose.header.frame_id = 'base_link'
            pose.header.frame_id = 'auto'

        else:
            pose.header.frame_id = 'map'
        
        pose.pose.position.x = x
        pose.pose.position.y = y
        pose.pose.position.z = z
        # pose.pose.orientation = Quaternion(*self.current_yaw)

        self.current_x = x
        self.current_y = y
        self.current_z = z
        

        return pose


    def getHeightFactor(self) -> int:
        return self.heightFactor
    # def set_orientation(self, yaw):
    #     pose = PoseStamped()
    #     pose.header.stamp = rospy.Time.now()
    #     if BODY_FLU:
    #         pose.header.frame_id = 'base_link'

    #     else:
    #         pose.header.frame_id = 'map'


    #     DEG_2_RAD = math.pi / 180.0
    #     pose.pose.position.x = self.current_x
    #     pose.pose.position.y = self.current_y
    #     pose.pose.position.z = self.current_z

        
    #     q = quaternion_from_euler(yaw*DEG_2_RAD, 0, yaw*DEG_2_RAD)
    #     print(Quaternion(*q))
    #     pose.pose.orientation = Quaternion(*q)

    #     self.current_yaw = q

    #     return pose

class Commander:

    AVAILABLE_DRONES    = ("gs0","gs1","gs2","gs3","gs4")
    INITIAL_POSITIONS   = (
        (0,0,1),
        (1,0,0),
        (2,0,0),
        (3,0,0),
        (4,0,0)
    )
    CONSTANT_Z = .8

    def __init__(self,instructions : list,looping = False):
        if not rospy.core.is_initialized():
            rospy.init_node("commander_node")
        rate = rospy.Rate(20)
        self.wayPoints = instructions
        self.loopDrones = looping
        self.numDrones = len(instructions)
        self.drones = []
        for x in range(self.numDrones):
            t = Commander.INITIAL_POSITIONS[x]
            self.drones.append(Follower(t[0],t[1],t[2],Commander.AVAILABLE_DRONES[x],x+1))
        self.currentWaypoint = 0

    def nextPosition(self) -> int:
        if(self.currentWaypoint >= len(self.wayPoints[0])):
            return -1
        i = 0
        for d in self.drones:
            waypoints = self.wayPoints[i][self.currentWaypoint]
            if(len(waypoints) == 3):
                d.move(waypoints[0], waypoints[1], waypoints[2])
            else:
                d.move(waypoints[0], waypoints[1], Commander.CONSTANT_Z * d.getHeightFactor())
            i += 1
        self.currentWaypoint += 1
        if(self.currentWaypoint >= len(self.wayPoints[0]) and self.loopDrones):
            self.currentWaypoint = 0
        return self.currentWaypoint

if __name__ == "__main__":

    option = -1
    try:
        option = int(input("Choose the option that you want:\n\t1. Read from CSV\n\t2. Read from ImgFile\n\t3. Read from WebCamera\n\t4. Read from DroneCamera \n"))
        if option < 1 or option > 4:
            raise Exception("Not an option")
    except Exception:
        print("please pick a valid option")

    
    instructions = []
    reader = Reader()

    if(option == 1): # CSV
        reader = CsvFileReader()
    elif(option == 2): # Img PNG
        reader = ImgFileReader()
    elif(option == 3): # WebCam
        reader = WebCamReader()
    elif(option == 4): # Drone Camera
        reader = DronCamReader()

    instructions = reader.readInstructions()

    c = Commander(instructions, False)
    time.sleep(1)
    try:
        while(c.nextPosition() >= 0):
                time.sleep(reader.WAIT_TIME)
    except KeyboardInterrupt:
        pass
    print("Recorrido concluido")