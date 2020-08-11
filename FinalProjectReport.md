# TASK FP. 1 Match 3D Objects
This section is addressed by doing an overall loop through the vector of DMatch (matched keypoints). With each set of matched keypoints, we find the bounding box for both the current frame and the previous frame. Once we find out this relationship of bounding boxes between two frame, we insert this important date into a multimap to keep track of it.

We want to find the bounding box relationship with the highest number with keypoints. The next routine uses the "forward relationship (which I define as current frame box to previous frame box)" to analyzes the weight (number of keypoints) associated with each bounding box. We are able to filter out many of the bounding box relationship that has small weight (e.g. keypoints enclosed).

At this point, however, we have only did half of the filtering. We then use the "reverse relationship (which I define as previous frame box to current frame box)" and analyze the weight and eliminate any redundant relationship.

At this point we should have the optimized bounding box relationship, and we will pass this information back to the program (store in pointed memory).

# TASK MP. 2 Compute Lidar-based TTC
This section is address by first finding the minimum distance. In order to filter out the outliers, we first sort the vector of lidar points with the custom sort function, and then we walk through the sorted vector to find the minimum value that can be validated by having neighboring values. We define a custom threshold (e.g 5cm) and and require a set number of neighbor (e.g 5 neigboring points) to be found before we consider this point to be valid.

We carry out this filter for both set of lidar points from current frame and previous frame. Once we have obtained the minimum distance values, we calculate the constant velocity based TTC based on the the instructor's equation.

# TASK MP. 3 Keypoint Removal
This section is addressed by referencing the roi member variable associated with each bounding box. We loop through the vector of matched keypoints and check if each keypoint is within the roi of the referenced bounding box. Once this check is passed, we uses the index to add the keypoints and matched keypoint set (DMatch) to the bounding box's member variable.

# TASK MP. 4 Keypoint Descriptors
This section is address by first iterate through the keypoints to find a list of distance pairs, calculate the distance ratio, and storing the data into a vector for future use.

We then find the median value of the vector, which is more robust than using the mean value.

# TASK MP. 5 Descriptor Matching
Three examples are investigated below:
## Example 1

# TASK MP. 6 Descriptor Distance Ratio
The KNN selection is implemented and can be selected by using the `SEL_KNN` string.




|Detector |Image|Remarks|
|--- |---|---|
|Shi-Tomasi|![detector-shitomashi](report_img/detector_shitomasi.png)|~120 keypoints detected. Keypoints are mainly on top, middle, and bottom.|
|Harris Corner|![detector-harris](report_img/detector_harris.png)|typically <40 keypoints detected, which is very low. Keypoints are on the spots with visible high contrast |
|FAST|![detector-FAST](report_img/detector_FAST.png)|~150 keypoints detected, which is slightly higher than Shi-Tomasi. More points from top to middle of the car.|
|BRISK|![detector-BRISK](report_img/detector_BRISK.png)|~270 keypoints detected, which makes it the most in the group. The keypoints are relatively more uniformly distributed.|
|ORB|![detector-ORB](report_img/detector_ORB.png)|~110 keypoints detected. Keypoints seem to cluster on two sides of the car. |
|AKAZE|![detector-AKAZE](report_img/detector_AKAZE.png)|~160 keypoints detected. Keypoints seems to cluster on two sides of the car. The distribution on Y axis seems relative more uniform. |
|SIFT|![detector-SIFT](report_img/detector_SIFT.png)|~135 keypoints detected. distribution looks like FAST |


# TASK MP.7

## Shi-Tomashi
| | Orignal detected keypoints | Points on preceding vehicle |
|--- |---:|---:|
| Image 0|1370|127|
| Image 1|1301|120|
| Image 2|1361|123|
| Image 3|1358|120|
| Image 4|1333|120|
| Image 5|1284|115|
| Image 6|1322|114|
| Image 7|1366|125|
| Image 8|1389|112|
| Image 9|1339|113|


## Harris Corner

| | Orignal detected keypoints | Points on preceding vehicle |
|--- |---:|---:|
| Image 0|115|17|
| Image 1|98|14|
| Image 2|113|18|
| Image 3|121|21|
| Image 4|160|26|
| Image 5|383|43|
| Image 6|85|18|
| Image 7|210|31|
| Image 8|171|26|
| Image 9|281|34|

## FAST

| | Orignal detected keypoints | Points on preceding vehicle |
|--- |---:|---:|
| Image 0|1824|149|
| Image 1|1832|152|
| Image 2|1810|152|
| Image 3|1817|157|
| Image 4|1793|149|
| Image 5|1796|150|
| Image 6|1788|157|
| Image 7|1695|152|
| Image 8|1749|139|
| Image 9|1770|144|

## BRISK
| | Orignal detected keypoints | Points on preceding vehicle |
|--- |---:|---:|
| Image 0|2757|254|
| Image 1|2777|274|
| Image 2|2741|276|
| Image 3|2735|275|
| Image 4|2757|293|
| Image 5|2695|275|
| Image 6|2715|289|
| Image 7|2628|268|
| Image 8|2639|260|
| Image 9|2672|250|

## ORB
| | Orignal detected keypoints | Points on preceding vehicle |
|--- |---:|---:|
| Image 0|500|91|
| Image 1|500|102|
| Image 2|500|106|
| Image 3|500|113|
| Image 4|500|109|
| Image 5|500|124|
| Image 6|500|129|
| Image 7|500|127|
| Image 8|500|124|
| Image 9|500|125|

## AKAZE
| | Orignal detected keypoints | Points on preceding vehicle |
|--- |---:|---:|
| Image 0|1351|162|
| Image 1|1327|157|
| Image 2|1311|159|
| Image 3|1351|154|
| Image 4|1360|162|
| Image 5|1347|163|
| Image 6|1363|173|
| Image 7|1331|175|
| Image 8|1357|175|
| Image 9|1331|175|

## SIFT
| | Orignal detected keypoints | Points on preceding vehicle |
|--- |---:|---:|
| Image 0|1438|137|
| Image 1|1371|131|
| Image 2|1380|121|
| Image 3|1335|135|
| Image 4|1305|134|
| Image 5|1370|139|
| Image 6|1396|136|
| Image 7|1382|147|
| Image 8|1463|156|
| Image 9|1422|135|


# TASK MP.8
Please see the following log output. For the purpose of such a benchmark, the main program has been refactored to use a function to allow detection and descriptor to be programmatically selected. This help to facilitate the large amount of possible combinations. Since it is also possible for detectors and descriptor to not work with each other and produce error, a try-catch block is also implemented to safeguard the program from terminating prematurely. The catch block could also allow the program to identify which combination would cause failure.

Note: since the task prescribes the BruteForced to be used, SIFT descriptor will not be able to work.

Note 2: It is also documented that AKAZE descriptor only works with AKAZE detectors.

[github.com/kyamagu/mexopencv/issues/351](https://github.com/kyamagu/mexopencv/issues/351)

[kyamagu.github.io/mexopencv/matlab/AKAZE.html](https://kyamagu.github.io/mexopencv/matlab/AKAZE.html)
