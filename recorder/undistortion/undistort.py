
import numpy as np
import cv2 as cv
import glob

pattern = (6, 9)
show_for = 10


# termination criteria
criteria = (cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_MAX_ITER, 30, 0.001)

# prepare object points, like (0,0,0), (1,0,0), (2,0,0) ....,(6,5,0)
objp = np.zeros((pattern[0]*pattern[1],3), np.float32)
objp[:,:2] = np.mgrid[0:pattern[0],0:pattern[1]].T.reshape(-1,2)
# Arrays to store object points and image points from all the images.
objpoints = [] # 3d point in real world space
imgpoints = [] # 2d points in image plane.
images = glob.glob('./pics/*.png')
for fname in images:
    img = cv.imread(fname)
    gray = cv.cvtColor(img, cv.COLOR_BGR2GRAY)
    # Find the chess board corners
    ret, corners = cv.findChessboardCornersSB(gray, pattern, cv.CALIB_CB_ADAPTIVE_THRESH
                                                           + cv.CALIB_CB_EXHAUSTIVE
                                                           + cv.CALIB_CB_FAST_CHECK)
    # If found, add object points, image points (after refining them)
    if ret == True:
        objpoints.append(objp)
        imgpoints.append(corners)
        # Draw and display the corners
        cv.drawChessboardCorners(img, pattern, corners, ret)
        print("good img")
        # cv.imshow('img', img)
        # cv.waitKey(show_for)
    else:
        print("poopy img")

cv.destroyAllWindows()


ret, mtx, dist, rvecs, tvecs = cv.calibrateCamera(objpoints, imgpoints, gray.shape[::-1], None, None)

# mtx = np.array([
#     [1.6995832269779798e+03, 0.0, 6.3895363170358996e+02],
#     [0.0, 1.6995832269779798e+03, 2.9574445327436030e+02],
#     [0.0, 0.0, 1.0]
# ])

# dist = np.array([-7.8186862583262018e-02, 3.7019103011721088e-01,
#             -9.8877132803925731e-03, 1.9574645436569896e-03, 0.0])

print(mtx, dist)


mean_error = 0
for i in range(len(objpoints)):
    imgpoints2, _ = cv.projectPoints(objpoints[i], rvecs[i], tvecs[i], mtx, dist)
    error = cv.norm(imgpoints[i], imgpoints2, cv.NORM_L2)/len(imgpoints2)
    mean_error += error
print( "total error: {}".format(mean_error/len(objpoints)) )