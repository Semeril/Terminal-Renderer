#include <bits/stdc++.h>
#include <windows.h>
#define WIDTH 120
#define RECT_WIDTH 60
#define HEIGHT 29
#define FILL '219'
#define FPS 60

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

using namespace std;

struct Rect
{
    double x, y, width, height;
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(double _x, double _y, double _w, double _h) : x(_x), y(_y), width(_w), height(_h) {}
};

struct Color
{
    int r, g, b;

    string code()
    {
        return "\033[38;2;" + to_string(r) + ';' + to_string(g) + ';' + to_string(b) + 'm';
    }

    Color(int _r, int _g, int _b)
        : r(_r), g(_g), b(_b)
    {
        if (_r > 255 || _g > 255 || _b > 255)
        {
            throw invalid_argument("Color value must be less than 256!");
        }
    }
    Color() : r(255), g(255), b(255) {}
};

struct Screen
{
    int width, height;
    vector<vector<Color>> _list;
    void draw(Rect &obj, Color color)
    {
        Rect printable = make_printable(obj);
        for (size_t dy = 0; dy < printable.height; dy++)
        {
            for (size_t dx = 0; dx < printable.width; dx++)
            {
                _list.at(printable.y + dy)[printable.x + dx] = color;
            }
        }
    }

    Rect make_printable(Rect obj)
    {
        obj.x = floor(obj.x * 2);
        obj.y = floor(obj.y);
        obj.width = floor(obj.width * 2);
        obj.height = floor(obj.height);
        int dw = (width) - (obj.x + obj.width);
        int dh = (height) - (obj.y + obj.height);
        if (dh < 0)
        {
            obj.height += dh;
        }
        if (obj.y < 0)
        {
            obj.height += obj.y;
        }
        if (dw < 0)
        {
            obj.width += dw;
        }
        if (obj.x < 0)
        {
            obj.width += obj.x;
        }
        obj.y = max(min(obj.y, (double)height), .0);
        obj.height = max(.0, obj.height);
        obj.x = max(min(obj.x, (double)width), .0);
        obj.width = max(.0, obj.width);
        return obj;
    }

    void show()
    {
        cout << "\033[H";
        string display;
        display.reserve(width * height * 20);
        for (auto &row : _list)
        {
            for (auto &color : row)
            {
                display += color.code() + (char)219;
                color = Color();
            }
            display += "\033[0m\n";
        }
        cout << display;
    }
    Screen(int w, int h) : width(w), height(h), _list(height, vector<Color>(width, Color())) {}
};

bool ispressed(int vk_code)
{
    return GetAsyncKeyState(vk_code) & 0x8000;
}

int main()
{

    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }

    Screen screen(WIDTH, HEIGHT);
    Rect box{RECT_WIDTH / 2, HEIGHT / 2, 1.5, 1};
    cout << "\033[2J\033[?25l";
    while (true)
    {
        if (ispressed('D'))
        {
            box.x += 0.5;
        }
        if (ispressed('A'))
        {
            box.x -= 0.5;
        }
        if (ispressed('W'))
        {
            box.y -= 0.5;
        }
        if (ispressed('S'))
        {
            box.y += 0.5;
        }
        screen.draw(box, Color(255, 0, 0));
        screen.show();
        Sleep(1000 / FPS);
    }

    return 0;
}