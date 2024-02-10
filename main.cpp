#include <X11/Xlib.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <list>


int WW = 800; //window width
int WH = 700; //window height
Window myWindow;
Display *ourDisplay;
int s;



class Brick
{
    Display *d;
    Window   w;
    int      s;
    int x;
    int y;
    int width, height;

public:
    Brick(Display *d, Window w, int s, int x , int y)
    : d(d), w(w), s(s), width(100), height(40), x(x), y(y) {};

    void Draw() const
    {
        XFillRectangle(d, w, DefaultGC(d,s), x, y, width, height);
    };

    bool CheckCollision(int& ball_x, int& ball_y, int& moveX, int& moveY)
    {
        int x_center = ball_x + 16/2;
        int y_center = ball_y + 16/2;

        if (ball_x >= x && ball_x <= x+width)
        {
            if (ball_y >= y && ball_y <= y+height) // попали в блок
            {
                //меняем направление мяча
                if (x_center > x + 25 && x_center < x+width - 25) moveY *= -1;
                else moveX *= -1;
                return 1;
            }
        }
        return 0;

    }
};

class Ball
{
    friend class Field;
    friend class Timer;
    Display *d;
    Window   w;
    int      s;
    int x;
    int y;
    int width, height;

public:
    Ball(Display *d, Window w, int s)
    : d(d), w(w), s(s), width(16), height(16)
    {
        x = WW / 2 +20;
        y = WH-120;
    };

    ~Ball(){};

    void Draw() const
    {
        XFillArc(d, w, DefaultGC(d,s), x, y, width, height, 0, 360*64);
    }
};

class Platform
{
    friend class Field;

    Display *d;
    Window   w;
    int      s;
    int x;
    int y;
    int width, height;
    int delta = 0;

public:
    Platform(Display *d, Window w, int s)
    :d(d), w(w), s(s), width(90), height(10)
    {
        y = WH - 90;
        x = WW / 2 - 10;
    };

    void Draw() const
    {
        XFillRectangle(d, w, DefaultGC(d,s), x, y, width, height);
    };

    bool goRight()
    {
        if ((x+20+width) < (WW)){
            delta = 20;
            return 0;
        }
        return 1;
        
    }

    bool goLeft()
    {
        if ((x) > (20)){
            delta = -20;
            return 0;
        }
        return 1;
        
    }
    ~Platform(){};
};

class Field
{
    friend class Timer;
    Display *d;
    Window   w;
    int      s;
    Platform& p;
    Ball& b; 
    std::list<Brick> Bricks;
    

    //Ball params
    int moveX = 2, moveY = -3;

    
public:
    Field(Display *d, Window w, int s, Platform& p, Ball& b, std::list<Brick>& Bricks)
    :d(d), w(w), s(s), p(p), b(b), Bricks(Bricks)
    {};

    void moveBall()
    {
        b.x += moveX;
        b.y += moveY;
    }

    void movePlatform()
    {
        if (((p.x+20+p.width) < (WW) || ((p.x) > (20)))) p.x += p.delta;
        p.delta = 0;
    }

    void CheckPlatformCollision()
    {
        if ((b.y +b.height) >= p.y && b.y <= p.y + p.height) //на уровне платформы
        {
            if (b.x +8  >= p.x && b.x + 8 <= p.x + p.width) moveY = moveY * (-1); // в пределах ширины платформы
        }
    }

    bool CheckWallCollision()
    {
        if (b.x <= 3) moveX = moveX * (-1); //левая стена 
        else if (b.x + b.width >= WW-3) moveX = moveX * (-1); //правая стена
        else if (b.y <= 3) moveY = moveY * (-1); //верхняя стена

        else if (b.y + b.height >= WH-3) // нижняя стена. проигрыш
        {
            moveX = 0;
            moveY = 0;   
            return 1;
        }
        return 0;
    }

    void DrawBricks()
    {
        for (std::list<Brick>::iterator it=Bricks.begin(); it!= Bricks.end(); ++it)
        {
            Brick br = *it;
            br.Draw();
        }
    }

    bool CheckBricksCollision()
    {
        for (std::list<Brick>::iterator it=Bricks.begin(); it!= Bricks.end(); ++it)
        {
            Brick br = *it;
            if (br.CheckCollision(b.x, b.y, moveX, moveY)) it =Bricks.erase(it);
            if (Bricks.size()== 0) break;
        }
        if (Bricks.size() == 0) return 1;
        return 0;

    }

    ~Field(){};
};

class Timer {
    size_t   term;
    void   (Field::*func)();
    bool     stop;
    Field* f;
    
  public:
    // Timer()
    //   : term(0), func(0), stop(false), x(0)
    // {
    //     x =0;
    //     y =0;


    // };

    Timer(void (Field::*func)(), Field* f)
    : term(0), func(func), stop(false), f(f)
    {}
    
    void StartTimer(size_t usec)
    {
      term = usec;
      
      struct timeval cur;
      struct timeval old;
      size_t delta;
      
      gettimeofday(&old, 0);
      while(!stop)
      {
        gettimeofday(&cur, 0);
	
	    delta  = cur.tv_sec  - old.tv_sec;
	    delta *= 10000;
	    delta += cur.tv_usec - old.tv_usec;
	
	    if(delta >= term)
	    {
	        if(func) (f->*func)();
	        stop = true;
	    }
      }
    }

    bool get_state()
    {
        return stop;
    }

    void set_state()
    {
        stop = false;
    }
};


int main()
{
    
    ourDisplay = XOpenDisplay(NULL);
    if ( ourDisplay == NULL) return 1;


    int s=DefaultScreen(ourDisplay);          // Экран по-умолчанию
    Window rootWindow=RootWindow(ourDisplay, s); // Корневое окно
    unsigned long bgcolor=WhitePixel(ourDisplay, s); 


    myWindow=XCreateSimpleWindow(ourDisplay,rootWindow,100, 100, WW, WH,
    0, 0, bgcolor);
    

    XSelectInput(ourDisplay,myWindow,
    KeyPressMask | KeyReleaseMask |
    ExposureMask );

    XMapWindow(ourDisplay, myWindow);    
    XFlush(ourDisplay);


    Platform p(ourDisplay,myWindow,s);
    Ball b(ourDisplay, myWindow,s);
    std::list<Brick> Bricks;
    int brick_x = 50, brick_y = 50;

 

    for (int i = 0; i < 3; i++)
    {
        Bricks.push_back(Brick(ourDisplay, myWindow, s, brick_x, brick_y));
        brick_x += 150;
    }

    Field f(ourDisplay,myWindow,s, p, b, Bricks);
    Timer t_ball(&Field::moveBall, &f);
    Timer t_pl(&Field::movePlatform, &f);
    t_ball.StartTimer(1);


    while (1)
    {

    

        XExposeEvent xe = {Expose, 0, 1, ourDisplay, myWindow, 0, 0, 
            WW, WH,
            0};
        XEvent event;
        XNextEvent(ourDisplay,&event);


        if (event.type == KeyPress)
        {
            if (event.xkey.keycode == 114) p.goRight();
            if (event.xkey.keycode == 113) p.goLeft();
        //             if (t_pl.get_state())
        // {
        //     t_pl.set_state();
        //     t_pl.StartTimer(1);
        // }
        }


        if (event.type == Expose)
        {

        if (t_ball.get_state())
        {
            t_ball.set_state();
            t_ball.StartTimer(20000);
        }

         
            //f.moveBall();
            f.movePlatform();
            f.CheckPlatformCollision();
            f.CheckBricksCollision();
        }

        XClearWindow(ourDisplay, myWindow);
        p.Draw();
        b.Draw();
        f.DrawBricks();

        if(!XPending(ourDisplay))
        {
        
        XSendEvent(ourDisplay, myWindow, False, ExposureMask, (XEvent *) &xe);
        usleep(1000);
        }





        if (f.CheckWallCollision()) 
        {
            std::cout << "Game Over";
            break;
        }

        if (f.CheckBricksCollision()) 
        {
            std::cout << "You won!";
            break;
        }



        XFlush(ourDisplay);
    }

    XDestroyWindow(ourDisplay, myWindow);

    XCloseDisplay(ourDisplay);
  
    return 0;
}