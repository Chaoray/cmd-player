# cmd-player

A simple command line video player.   
It uses opencv as decoder and uses winapi to boost drawing performance.  
Also using multithreading to decode and draw at the same time.

Most of the time it can keep up with the video frame rate.  
However, it may drop frames when the render size is too large.

## Usage
```shell
cmd-player.exe <video file>
```

## Key bindings
- `q` or `ESC`: Quit
