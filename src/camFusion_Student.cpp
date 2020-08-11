
#include <iostream>
#include <algorithm>
#include <numeric>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "camFusion.hpp"
#include "dataStructures.h"
#include <unordered_set>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;


// Create groups of Lidar points whose projection into the camera falls into the same bounding box
void clusterLidarWithROI(std::vector<BoundingBox> &boundingBoxes, std::vector<LidarPoint> &lidarPoints, float shrinkFactor, cv::Mat &P_rect_xx, cv::Mat &R_rect_xx, cv::Mat &RT)
{
    // loop over all Lidar points and associate them to a 2D bounding box
    cv::Mat X(4, 1, cv::DataType<double>::type);
    cv::Mat Y(3, 1, cv::DataType<double>::type);

    for (auto it1 = lidarPoints.begin(); it1 != lidarPoints.end(); ++it1)
    {
        // assemble vector for matrix-vector-multiplication
        X.at<double>(0, 0) = it1->x;
        X.at<double>(1, 0) = it1->y;
        X.at<double>(2, 0) = it1->z;
        X.at<double>(3, 0) = 1;

        // project Lidar point into camera
        Y = P_rect_xx * R_rect_xx * RT * X;
        cv::Point pt;
        pt.x = Y.at<double>(0, 0) / Y.at<double>(0, 2); // pixel coordinates
        pt.y = Y.at<double>(1, 0) / Y.at<double>(0, 2);

        vector<vector<BoundingBox>::iterator> enclosingBoxes; // pointers to all bounding boxes which enclose the current Lidar point
        for (vector<BoundingBox>::iterator it2 = boundingBoxes.begin(); it2 != boundingBoxes.end(); ++it2)
        {
            // shrink current bounding box slightly to avoid having too many outlier points around the edges
            cv::Rect smallerBox;
            smallerBox.x = (*it2).roi.x + shrinkFactor * (*it2).roi.width / 2.0;
            smallerBox.y = (*it2).roi.y + shrinkFactor * (*it2).roi.height / 2.0;
            smallerBox.width = (*it2).roi.width * (1 - shrinkFactor);
            smallerBox.height = (*it2).roi.height * (1 - shrinkFactor);

            // check wether point is within current bounding box
            if (smallerBox.contains(pt))
            {
                enclosingBoxes.push_back(it2);
            }

        } // eof loop over all bounding boxes

        // check wether point has been enclosed by one or by multiple boxes
        if (enclosingBoxes.size() == 1)
        { 
            // add Lidar point to bounding box
            enclosingBoxes[0]->lidarPoints.push_back(*it1);
        }

    } // eof loop over all Lidar points
}

void showLidarTopview(std::vector<LidarPoint> &lidarPoints, cv::Size worldSize, cv::Size imageSize, int imageIndex)
{
    // create topview image
    cv::Mat topviewImg(imageSize, CV_8UC3, cv::Scalar(0, 0, 0));

    // plot Lidar points into image
    for (auto it = lidarPoints.begin(); it != lidarPoints.end(); ++it)
    {
        float xw = (*it).x; // world position in m with x facing forward from sensor
        float yw = (*it).y; // world position in m with y facing left from sensor

        int y = (-xw * imageSize.height / worldSize.height) + imageSize.height;
        int x = (-yw * imageSize.height / worldSize.height) + imageSize.width / 2;

        float zw = (*it).z; // world position in m with y facing left from sensor
        if(zw > -1.40){       

            float val = it->x;
            float maxVal = worldSize.height;
            int red = min(255, (int)(255 * abs((val - maxVal) / maxVal)));
            int green = min(255, (int)(255 * (1 - abs((val - maxVal) / maxVal))));
            cv::circle(topviewImg, cv::Point(x, y), 5, cv::Scalar(0, green, red), -1);
        }
    }

    // plot distance markers
    float lineSpacing = 2.0; // gap between distance markers
    int nMarkers = floor(worldSize.height / lineSpacing);
    for (size_t i = 0; i < nMarkers; ++i)
    {
        int y = (-(i * lineSpacing) * imageSize.height / worldSize.height) + imageSize.height;
        cv::line(topviewImg, cv::Point(0, y), cv::Point(imageSize.width, y), cv::Scalar(255, 0, 0));
    }

    // display image
    string windowName = "Top-View Perspective of LiDAR data";
    cv::namedWindow(windowName, 2);
    cv::imshow(windowName, topviewImg);

    string imageString = "./TopImage/TopViewImage_";
    if(imageIndex < 10)
        imageString += "0";
    imageString += to_string(imageIndex);
    imageString += ".jpg";
    cv::imwrite(imageString,topviewImg );
    //cv::waitKey(0); // wait for key to be pressed
}

void show3DObjects(std::vector<BoundingBox> &boundingBoxes, cv::Size worldSize, cv::Size imageSize, bool bWait, int imageIndex)
{
    // create topview image
    cv::Mat topviewImg(imageSize, CV_8UC3, cv::Scalar(255, 255, 255));
    //cv::circle(topviewImg, cv::Point(10,10), 4, cv::Scalar(0,1,0), 2);
    //std::cout << "topviewimg row:" << topviewImg.rows << ",col: " << topviewImg.cols << endl;

    for(auto it1=boundingBoxes.begin(); it1!=boundingBoxes.end(); ++it1)
    {
        // create randomized color for current 3D object
        cv::RNG rng(it1->boxID);
        //cv::Scalar currColor = cv::Scalar(rng.uniform(0,150), rng.uniform(0, 150), rng.uniform(0, 150));
        cv::Scalar currColor = cv::Scalar(0,1,0);

        // plot Lidar points into top view image
        int top=1e8, left=1e8, bottom=0.0, right=0.0; 
        float xwmin=1e8, ywmin=1e8, ywmax=-1e8;
        for (auto it2 = it1->lidarPoints.begin(); it2 != it1->lidarPoints.end(); ++it2)
        {
            // world coordinates
            float xw = (*it2).x; // world position in m with x facing forward from sensor
            float yw = (*it2).y; // world position in m with y facing left from sensor
            xwmin = xwmin<xw ? xwmin : xw;
            ywmin = ywmin<yw ? ywmin : yw;
            ywmax = ywmax>yw ? ywmax : yw;

            // top-view coordinates
            int y = (-xw * imageSize.height / worldSize.height) + imageSize.height;
            int x = (-yw * imageSize.width / worldSize.width) + imageSize.width / 2;

            // find enclosing rectangle
            top = top<y ? top : y;
            left = left<x ? left : x;
            bottom = bottom>y ? bottom : y;
            right = right>x ? right : x;

            // draw individual point
            //cv::circle(topviewImg, cv::Point(x, y), 4, currColor, -1);
            cv::circle(topviewImg, cv::Point(x, y), 4, currColor, 2);
            //std::cout << "draw circle: x:" << x << ",y: " << y << endl;
        }

        // draw enclosing rectangle
        cv::rectangle(topviewImg, cv::Point(left, top), cv::Point(right, bottom),cv::Scalar(0,0,0), 2);

        // augment object with some key data
        char str1[200], str2[200];
        sprintf(str1, "id=%d, #pts=%d", it1->boxID, (int)it1->lidarPoints.size());
        putText(topviewImg, str1, cv::Point2f(left-250, bottom+50), cv::FONT_ITALIC, 2, currColor);
        sprintf(str2, "xmin=%2.2f m, yw=%2.2f m", xwmin, ywmax-ywmin);
        putText(topviewImg, str2, cv::Point2f(left-250, bottom+125), cv::FONT_ITALIC, 2, currColor);  
    }

    //std::cout << "checkpoint 1" << endl;

    // plot distance markers
    float lineSpacing = 2.0; // gap between distance markers
    int nMarkers = floor(worldSize.height / lineSpacing);
    for (size_t i = 0; i < nMarkers; ++i)
    {
        int y = (-(i * lineSpacing) * imageSize.height / worldSize.height) + imageSize.height;
        cv::line(topviewImg, cv::Point(0, y), cv::Point(imageSize.width, y), cv::Scalar(255, 0, 0));
    }

    
    //std::cout << "checkpoint 2" << endl;
    // display image
    string windowName = "3D Objects";
    cv::namedWindow(windowName, 1);
    cv::imshow(windowName, topviewImg);
    string imageString = "./3DImage/3DViewImage_";
    imageString += to_string(imageIndex);
    imageString += ".jpg";
    cv::imwrite(imageString,topviewImg );
     //std::cout << "checkpoint 3" << endl;
    if(bWait)
    {
        cv::waitKey(0); // wait for key to be pressed
    }
}



void matchBoundingBoxes(std::vector<cv::DMatch> &matches, std::map<int, int> &bbBestMatches, DataFrame &prevFrame, DataFrame &currFrame)
{
    //input variables
    //std::vector<cv::DMatch> &matches
    //std::map<int, int> &bbBestMatches
    //DataFrame &prevFrame
    //DataFrame &currFrame

    std::multimap<int,int> Curr2Prev_BB;
    //std::multimap<int,int> Prev2Curr_BB;
    //std::map<std::map<int>> Associated_BB;
    std::unordered_set<int> relevantKeyPt;
    
    for(cv::DMatch matchPoint : matches)
    {
        //cout << "queryIdx(currentImg): " << matchPoint.queryIdx <<
        //    " trainIdx(prevImg): " << matchPoint.trainIdx << endl;
        //cout << "current keypoint x: " << currFrame.keypoints[matchPoint.queryIdx].pt.x << endl;
        int key_x1 = currFrame.keypoints[matchPoint.trainIdx].pt.x; //queryIdx
        int key_y1 = currFrame.keypoints[matchPoint.trainIdx].pt.y;
        int key_x2 = prevFrame.keypoints[matchPoint.queryIdx].pt.x; //trainIdx
        int key_y2 = prevFrame.keypoints[matchPoint.queryIdx].pt.y;  
        //for()
        for(vector<BoundingBox>::iterator curBB_it = currFrame.boundingBoxes.begin(); curBB_it != currFrame.boundingBoxes.end(); curBB_it++)
        {
            //cout << "BB key size: " << BB_it->roi.x << endl;
            //cout << "BB key size: " << BB_it->keypoints.size() << endl;
            
            //iterate through all boxing box. If point is within then add to the list
            if(key_x1 >= curBB_it->roi.x && key_x1 <= (curBB_it->roi.x+curBB_it->roi.width) &&
                key_y1 >= curBB_it->roi.y && key_y1 <= (curBB_it->roi.y+curBB_it->roi.height))
            {
                //cout << "keypoint in current frame BB" << (curBB_it - currFrame.boundingBoxes.begin()) <<endl;
                
                //BB_it->keypoints.push_back(currFrame.keypoints[matchPoint.queryIdx]);

                for(vector<BoundingBox>::iterator prevBB_it = prevFrame.boundingBoxes.begin(); prevBB_it != prevFrame.boundingBoxes.end(); prevBB_it++)
                {
                    if(key_x2 >= prevBB_it->roi.x && key_x2 <= (prevBB_it->roi.x+prevBB_it->roi.width) &&
                        key_y2 >= prevBB_it->roi.y && key_y2 <= (prevBB_it->roi.y+prevBB_it->roi.height))
                    {
                        //cout << "keypoint in BB" << endl;
                        //BB_it->keypoints.push_back(prevFrame.keypoints[matchPoint.queryIdx]);
                        //cout << "keypoint in previous frame BB" << (prevBB_it - prevFrame.boundingBoxes.begin()) <<endl;
                        Curr2Prev_BB.insert(std::make_pair((curBB_it - currFrame.boundingBoxes.begin()), (prevBB_it - prevFrame.boundingBoxes.begin())));
                        //Prev2Curr_BB.insert(std::make_pair((prevBB_it - prevFrame.boundingBoxes.begin())),(curBB_it - currFrame.boundingBoxes.begin()));
                        relevantKeyPt.insert((curBB_it - currFrame.boundingBoxes.begin()));
                    } 
                }
            }
            
        }

      
    }

    std::unordered_set<int> PointsForReverseCheck;
    std::vector<std::vector<int>> forwardBestMatch;
    // now find the bounding box with most keypoints
    for(int tempInt : relevantKeyPt)
    {
        //std::cout << "Index: " << tempInt << " has size: " <<
        //    Associated_BB.count(tempInt) << endl;

        std::multimap<int,int> tempMap;
        std::unordered_set<int> tempSet;
        // for this particular bounding box index. find the matching bb with highest occurance
        std::multimap<int,int>::iterator mm_it;
        for(mm_it = Curr2Prev_BB.begin(); mm_it != Curr2Prev_BB.end(); mm_it++)
        {
            //std::cout << mm_it->second << endl;
            if(mm_it->first == tempInt)
            {
                tempMap.insert(std::make_pair(mm_it->second,0));
                tempSet.insert(mm_it->second);
            }

        }
        int testIndex = *(tempSet.begin());
        int textIndexCount = tempMap.count(testIndex);
        //std::cout << "testIndex starts at: " << testIndex << endl;
        for(int tempInd : tempSet)
        {
            //std::cout << "index: (curr)" << tempInt << 
            //    "index2: (prev)" << tempInd << " with count: " << tempMap.count(tempInd) << endl;
            if(tempMap.count(tempInd) > textIndexCount)
            {
                testIndex = tempInd;
                textIndexCount = tempMap.count(tempInd);
                
            }
        }

        // now we should have lagest value
        //std::cout << "best matches: (curr)" << tempInt << " with: (prev)" << 
        //    testIndex << " point count:" << textIndexCount << endl;
        forwardBestMatch.push_back({tempInt,testIndex,textIndexCount});
        //forwardBestMatch.insert(std::make_pair(tempInt,tempInt));
        PointsForReverseCheck.insert(testIndex);

    }

    //reverse check
    std::multimap<int,int> totalBestMatch;
    for(int test_prevBBindex : PointsForReverseCheck)
    {
        int currentWeight=0;
        int test_currBBindex = 0;

        //cout << "currenting testing index: " << test_prevBBindex << endl;

        std::vector<std::vector<int>>::iterator vector_it;
        for(vector_it = forwardBestMatch.begin(); vector_it != forwardBestMatch.end(); vector_it++)
        {
            std::vector<int> testVec = *vector_it;
            
            if(testVec[1] == test_prevBBindex)
            {
                int testValue = testVec[2];
                //std::cout << "weight: " << testValue << " against: " << currentWeight  << endl;
                if(currentWeight < testValue)
                {
                    currentWeight = testValue;
                    //std::cout << "updated weight is: " << currentWeight << endl;;
                    test_currBBindex = testVec[0];
                    //std::cout << "higher weight found at index: " << test_currBBindex << endl;
                }

            }
            
            
        }

        //After this, we should have best case, and can store to 
        bbBestMatches.insert(std::pair<int,int>(test_prevBBindex, test_currBBindex)); //order: prev, current
    }
    

}

bool CompareLidarPointFuncX(LidarPoint i, LidarPoint j)
{
    return (i.x < j.x);
}

void computeTTCLidar(std::vector<LidarPoint> &lidarPointsPrev,
                     std::vector<LidarPoint> &lidarPointsCurr, double frameRate, double &TTC)
{
    // Find the minimum X for both curr and prev by first sort then using a threshold
    std::vector<LidarPoint> lidarPointsPrev2(lidarPointsPrev);
    std::sort(lidarPointsPrev2.begin(), lidarPointsPrev2.end(), CompareLidarPointFuncX);

    // start testing the short dist point, and check if it has 5 neighbot point to validate
    double minPrevX = lidarPointsPrev2.begin()->x;
    int ValidateCounter = 0;
    double validateThreshold = 0.05; // must be within 5cm
    int ValidateRequirement = 5;

    for(LidarPoint testPt : lidarPointsPrev2)
    {
        //cout << testPt.x << endl;
        if((abs(minPrevX - testPt.x)) < validateThreshold)
        {
            if(++ValidateCounter > 5)
                break;
        }
        else
            minPrevX = testPt.x;
    }

    std::vector<LidarPoint> lidarPointsCurr2(lidarPointsCurr);
    std::sort(lidarPointsCurr2.begin(), lidarPointsCurr2.end(), CompareLidarPointFuncX);
    double minCurrX = lidarPointsCurr2.begin()->x;
    ValidateCounter = 0;
    for(LidarPoint testPt : lidarPointsCurr2)
    {
        //cout << testPt.x << endl;
        if((abs(minCurrX - testPt.x)) < validateThreshold)
        {
            if(++ValidateCounter > 5)
                break;
        }
        else
            minCurrX = testPt.x;
    }


    TTC = 1/frameRate * minCurrX /(minPrevX - minCurrX);
    //cout << "Min PrevX: " << minPrevX << " CurrX: " << minCurrX <<
    //    "DeltaX: " << (minPrevX-minCurrX) << " TTC: " << TTC << endl;


}


// associate a given bounding box with the keypoints it contains
void clusterKptMatchesWithROI(BoundingBox &boundingBox, std::vector<cv::KeyPoint> &kptsPrev, std::vector<cv::KeyPoint> &kptsCurr, std::vector<cv::DMatch> &kptMatches)
{
    for(cv::DMatch matchPoint : kptMatches)
    {
        int key_x1 = kptsCurr[matchPoint.trainIdx].pt.x;
        int key_y1 = kptsCurr[matchPoint.trainIdx].pt.y;

        if(key_x1 >= boundingBox.roi.x && key_x1 <= (boundingBox.roi.x+boundingBox.roi.width) &&
            key_y1 >= boundingBox.roi.y && key_y1 <= (boundingBox.roi.y+boundingBox.roi.height))
        {
            //cout << "keypoint is in bounding box" << endl;
            boundingBox.keypoints.push_back(kptsCurr[matchPoint.trainIdx]);
            boundingBox.kptMatches.push_back(matchPoint);
        }
        else
        {
            //cout << "keypoint not in bounding box" << endl;
        }
        

    }
}

void computeTTCCamera2(std::vector<cv::KeyPoint> &kptsPrev, std::vector<cv::KeyPoint> &kptsCurr,
    std::vector<cv::DMatch> kptMatches, double frameRate, double &TTC, cv::Mat *visImg)
{
      // compute distance ratios between all matched keypoints
    vector<double> distRatios; // stores the distance ratios for all keypoints between curr. and prev. frame
    for (auto it1 = kptMatches.begin(); it1 != kptMatches.end() - 1; ++it1)
    { // outer kpt. loop

        // get current keypoint and its matched partner in the prev. frame
        cv::KeyPoint kpOuterCurr = kptsCurr.at(it1->trainIdx);
        cv::KeyPoint kpOuterPrev = kptsPrev.at(it1->queryIdx);

        for (auto it2 = kptMatches.begin() + 1; it2 != kptMatches.end(); ++it2)
        { // inner kpt.-loop

            double minDist = 100.0; // min. required distance

            // get next keypoint and its matched partner in the prev. frame
            cv::KeyPoint kpInnerCurr = kptsCurr.at(it2->trainIdx);
            cv::KeyPoint kpInnerPrev = kptsPrev.at(it2->queryIdx);

            // compute distances and distance ratios
            double distCurr = cv::norm(kpOuterCurr.pt - kpInnerCurr.pt);
            double distPrev = cv::norm(kpOuterPrev.pt - kpInnerPrev.pt);

            if (distPrev > std::numeric_limits<double>::epsilon() && distCurr >= minDist)
            { // avoid division by zero

                double distRatio = distCurr / distPrev;
                distRatios.push_back(distRatio);
            }
        } // eof inner loop over all matched kpts
    }     // eof outer loop over all matched kpts

    // only continue if list of distance ratios is not empty
    if (distRatios.size() == 0)
    {
        TTC = NAN;
        return;
    }


    std::sort(distRatios.begin(), distRatios.end());
    long medIndex = floor(distRatios.size() / 2.0);
    double medDistRatio = distRatios.size() % 2 == 0 ? (distRatios[medIndex - 1] + distRatios[medIndex]) / 2.0 : distRatios[medIndex]; // compute median dist. ratio to remove outlier influence

    double dT = 1 / frameRate;
    TTC = -dT / (1 - medDistRatio);
}
