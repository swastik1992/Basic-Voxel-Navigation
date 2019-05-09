# Basic-Voxel-Navigation
------
## [WIP] This is a simple 3d/surface navigation sample.
Note: Since it's work in progress, there are many known and unkonwn issues which I am trying to solve when ever I get time.

The idea behind voxel navigation is very simple, do a pathfind in 3d. The navigation could just be on a surface for a 3d object or 
in 3d space. 
Appilcations for this could be doing a 3d pathfind for thor's hammer 'Mjolnir' to travel back, path calculation for wall running, making a bug walk on any 3d surface, pathfinding for flying cars etc. 

#### Brief steps in making the voxel navigation:
* Creating voxels for defined volume (cube). 
* Checking for overlapped voxels, surface voxels. Surface voxels are simply all the valid neighbours of an overlapped voxel.
Once the generator is done creating voxels for a defined volume, the next step would be to create a pathfinder. 
A pathfinder is an actor component, which takes a navigation query and does A*/Bi-directional pathfinding on it.

18 voxel neighbours were used for pathfinding and for finding surface voxels.
![Voxel Neighbours](https://raw.githubusercontent.com/swastik1992/Basic-Voxel-Navigation/master/Images/neighboursNew.png)

* Navigation query starts by creating a task which would hold a result handler for result callback and every data needed to start the navigation. (start location, end location, final path, start voxel, end voxel, priority queue etc.)
* All the tasks will be placed in a queue and will run according to their age. Only a certain number of tasks would be able to run every tick so we don't work with too many tasks on a single frame.
* Furthermore, each task will be divided over into multiple frames, in case we are doing a navigation query on a very complicated or big scene, we don't want to completely occupy the cpu cyles for the current frame. With that in mind, we can set a limit for iteration for each task. (since pathfinding requires to run a loop on all the nodes/voxels, we can count each loop iteration as iteration for the task.
* Priority queue is the most importatnt part of pathfinding, as we store the smallest/ closest voxel to target into our priority queue (On each iteration, we check for the lowest weight voxel from priority queue and propagate by choosing the next priority queue voxel from the 18 neighbours of lowest weight voxel). 
* Priority queue that is used in this example is a simple std::priority_queue. It's priority has been changed by using std::greater<> so the smallest weight element would appear at the top. 
* Once the top element of priority queue becomes the target voxel, pathfinding for the related task stops and handler does a broadcast for the final path data. 
-------
All the examples below had their tick interval increased (0.1 - 0.5), max task per tick was set to 1 and total number of iteration per tick was set to max 10. This was all done just for debugging and viewing purposes. On release build, tick interval would set to default engine tick interval, max task per tick would be set to something between 100 to 500 and max iteration per tick would be atleast 1000. 

#### 3D Pathfinding example:
![Voxel Neighbours](https://raw.githubusercontent.com/swastik1992/Basic-Voxel-Navigation/master/Images/3d_demo.gif)
#### 3D bi-directional pathfinding example:
![Voxel Neighbours](https://raw.githubusercontent.com/swastik1992/Basic-Voxel-Navigation/master/Images/bi-dir_demo.gif)
#### Surface Pathfinding example:
![Voxel Neighbours](https://raw.githubusercontent.com/swastik1992/Basic-Voxel-Navigation/master/Images/Surface_demo2.gif)
#### Surface bi-direction pathfinding example:
![Voxel Neighbours](https://raw.githubusercontent.com/swastik1992/Basic-Voxel-Navigation/master/Images/bi-dirSurface_demo1.gif)
![Voxel Neighbours](https://raw.githubusercontent.com/swastik1992/Basic-Voxel-Navigation/master/Images/bi-dirSurface_demo5.gif)












