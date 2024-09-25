#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <Windows.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include "color2ascii.h"

HANDLE hStdout;

SIZE buffer_size;
SIZE frame_size;
SIZE render_size;
int rendered_frame_count = 0;

cv::VideoCapture capture;
std::queue<char*> frame_buffer;
bool is_video_end = false;
bool is_paused = false;

std::mutex g_mutex;

void set_cursor_visible(bool visible) {
    CONSOLE_CURSOR_INFO cursor_info;
    GetConsoleCursorInfo(hStdout, &cursor_info);
    cursor_info.dwSize = 1;
    cursor_info.bVisible = visible;
    SetConsoleCursorInfo(hStdout, &cursor_info);
}

void get_screen_size() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdout, &csbi);
    buffer_size.cx = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    buffer_size.cy = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

void calculate_size() {
    int cols = capture.get(cv::CAP_PROP_FRAME_WIDTH), rows = capture.get(cv::CAP_PROP_FRAME_HEIGHT);
    frame_size.cx = cols;
    frame_size.cy = rows;

    if (rows > cols) {
        rows = buffer_size.cy - 1;
        cols = rows * frame_size.cx / frame_size.cy * 2 - 1;
    } else {
        cols = buffer_size.cx - 1;
        rows = cols * frame_size.cy / frame_size.cx / 2 - 1;
        if (rows > buffer_size.cy - 1) {
            rows = buffer_size.cy - 1;
            cols = rows * frame_size.cx / frame_size.cy * 2 - 1;
        }
    }
    render_size.cx = cols;
    render_size.cy = rows;
}

void video_process() {
    int rows = render_size.cy, cols = render_size.cx;
    cv::Mat frame;

    while (true) {
        capture >> frame;
        if (frame.empty())
            break;

        cv::resize(frame, frame, { cols, rows }, 0, 0, cv::INTER_NEAREST);
        char* single_frame = new char[rows * cols];

        int r, g, b;
        for (int i = 0; i < rows; i++) {
            uchar* pixel = frame.ptr<uchar>(i);
            for (int j = 0; j < cols; j++) {
                b = *pixel++;
                g = *pixel++;
                r = *pixel++;
                single_frame[i * cols + j] = color2ascii(r, g, b);
            }
        }

        g_mutex.lock();
        frame_buffer.push(single_frame);
        g_mutex.unlock();

        rendered_frame_count++;
    }

    is_video_end = true;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <video file>\nSpace Bar: pause\nESC: quit\n", argv[0]);
        return 1;
    }

    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    capture.open(argv[1]);
    if (!capture.isOpened()) {
        printf("Error: cannot open video file \"%s\"\n", argv[1]);
        return 1;
    }

    get_screen_size();
    calculate_size();

    std::thread video_process_thread(video_process);
    video_process_thread.detach();

    auto delay = std::chrono::nanoseconds(int(1e9 / capture.get(cv::CAP_PROP_FPS)));
    auto next = std::chrono::high_resolution_clock::now() + delay;
    auto fps_start = std::chrono::high_resolution_clock::now();
    auto fps_end = std::chrono::high_resolution_clock::now();
    auto video_start_time = std::chrono::steady_clock::now();

    float fps = 0.0f;
    int frame_count = 0;
    long long elapsed_time = 0;
    size_t buffer_size = render_size.cx * render_size.cy;
    DWORD lpNumberOfCharsWritten;

    set_cursor_visible(false);

    while (true) {
        if (frame_buffer.empty()) { [[unlikely]]
            if (is_video_end) { [[unlikely]]
                break;
            }
            continue;
        }

        g_mutex.lock();
        char* single_frame = frame_buffer.front();
        if (!is_paused) [[likely]]
            frame_buffer.pop();
        g_mutex.unlock();

        // draw info
        elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - video_start_time).count();
        sprintf_s(single_frame, buffer_size, "  Video Size: %dx%d  Render Size: %dx%d  FPS: %.2f  Elapsed Time: %lldms  Frame Number: %d  Rendered Frame: %d  ", frame_size.cx, frame_size.cy, render_size.cx, render_size.cy, fps, elapsed_time, frame_count, rendered_frame_count);

        // draw frame
        for (short i = 0; i < render_size.cy; i++) {
            SetConsoleCursorPosition(hStdout, { 0, i });
            WriteConsoleA(hStdout, single_frame + i * render_size.cx, render_size.cx, &lpNumberOfCharsWritten, NULL);
        }

        if (!is_paused) [[likely]]
            delete[] single_frame;

        // busy sleep
        while (std::chrono::high_resolution_clock::now() < next) {
            std::this_thread::yield();
        }
        next = std::chrono::high_resolution_clock::now() + delay;

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) [[unlikely]]
            break;
        if (GetAsyncKeyState(0x51) & 0x8000) [[unlikely]] // Q
            break;

        // fps counter
        fps_end = std::chrono::high_resolution_clock::now();
        fps = 1e9 / (fps_end - fps_start).count();
        fps_start = std::chrono::high_resolution_clock::now();

        if (!is_paused) [[likely]]
            frame_count++;
    }

    set_cursor_visible(true);
}
