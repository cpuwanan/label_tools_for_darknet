# label_tools_for_darknet
Label tools for image training by YOLO Darknet

# Ex 1: Usage for video label
```
$ cd build
$ cmake ..
$ make
$ ./label_tools_video --config ../config/label_tools_video.yaml
```
- Usage guide

| Key | Description                                    |
|-----|------------------------------------------------|
| ESC | to quit the software                           |
| q   | to play the next video file                    |
| p   | 'TOGGLE' to play or pause the video            |
| s   | to set the starting frame No. for video play   |
| f   | to set the labels on the current image         |
| a   | to add the labeled box to the database         |
| 1   | to display 'ID' to all labeled boxes           |
| d   | to delete a labeled box by 'ID'                |
| r   | to reset polygon position                      |

# Ex 2: Usage for label checker
```
$ cd build
$ cmake ..
$ make
$ ./label_tools_image --config ../config/label_tools_image.yaml
```
- Usage guide

| Key | Description                                    |
|-----|------------------------------------------------|
| ESC | to quit the software                           |
| q   | to show the next image                         |
| w   | to show the previous image                     |
| a   | to add the labeled box to the database         |
| 1   | to display 'ID' to all labeled boxes           |
| d   | to delete a labeled box by 'ID'                |
| f   | to delete the last labeled box                 |
| r   | to reset polygon position                      |
