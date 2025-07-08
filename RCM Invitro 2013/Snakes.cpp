
#include <vector>
#include <cstdlib>
#include <algorithm>

static inline double random(double r)
{
    return double(rand()) / double(RAND_MAX);
}

struct Coord
{
   int x, y;

   Coord(): x(0), y(0)
   {
   }

   Coord(int x, int y): x(x), y(y)
   {
   }

   int& operator[](size_t idx)
   {
      return idx ? y : x;
   }

   const int& operator[](size_t idx) const
   {
      return idx ? y : x;
   }
};

struct Snake
{
  static const int UP = 0;
  static const int LEFT = 1;
  static const int RIGHT = 2;
  static const int DOWN = 3;

  Snake()
  {
    reset();
  }

  Coord nextPos()
  {
    switch(dir)
    {
    case UP:
      return (Coord){
        pos[0], pos[1] - 1
      };
    case DOWN:
      return (Coord){
        pos[0], pos[1] + 1
      };
    case LEFT:
      return (Coord){
        pos[0] - 1, pos[1]
      };
    case RIGHT:
      return (Coord){
        pos[0] + 1, pos[1]
      };
    }

    return pos;
  }

  void reset()
  {
    active = true;
    black = false;
    pos[0] = pos[1] = 0;
    start_pos[0] = start_pos[1] = 0;
    dir = 0;
    id = 0;
    step_num = 32768;
    turn_num = 0;
    going_backwards = false;
  }

  bool active;
  bool black;
  Coord pos, start_pos;
  int dir;
  int id;
  int step_num;
  int turn_num;
  bool going_backwards;
};


struct Maze
{
  typedef std::vector<Snake> SnakeCollection;

  Maze(int w, int h, int num_snakes)
  {
    this->w = w;
    this->h = h;
    occupied = new int[w * h];
    blackgrid = new bool[w * h];
    restarts = 0;
    snake_id_counter = 1;

    addSomeSnakes(num_snakes);
  }

  void addSomeSnakes(int n)
  {
    for (int i = 0; i < n; ++i)
    {
      int x, y;

      do
      {
        x = int(random(w));
        y = int(random(h));

      } while (snakeOverlaps (x, y));

      createSnake(x, y);
    }
  }

  bool snakeOverlaps(int x, int y)
  {
    for (SnakeCollection::iterator s = snakes.begin(); s != snakes.end(); ++s)
    {
      if (x == s->pos[0] && y == s->pos[1])
        return true;
    }
    return false;
  }

  void createSnake(int x, int y)
  {
    Snake* s = 0;

    for(int j = 0; j < snakes.size(); ++j)
      if(snakes[j].active == false)
      {
        s = &snakes[j];
        s->reset();
        s->id = snake_id_counter;
        ++snake_id_counter;
        break;
      }

    if(s == 0)
    {
      snakes.push_back(Snake());
      s = &snakes.back();
    }

    s->pos[0] = x;
    s->pos[1] = y;
    s->start_pos[0] = s->pos[0];
    s->start_pos[1] = s->pos[1];
    s->dir = int(random(4));
    //s.black = random(1) < 0.5;

    s->id = snake_id_counter;
    ++snake_id_counter;
  }

  bool clear(int x, int y)
  {
    if (x < 0 || y < 0 || x >= w || y >= h)
      return false;

    return occupied[x + y * w] == 0;
  }

  /*
  void draw()
  {
    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; ++x)
      {
        int id = occupied[x + y * w];
        bool black = blackgrid[x + y * w];
        if (id > 0)
        {
          fill(255);
          noStroke();
          colorMode(HSB, 255);

          int m = 1;

          if(black)
          {
            fill(20);
            m = 0;
          }
          else
            fill(color((id >> 16) * 10 & 255, 126, int((cos(frameCount * 0.1 + id * 0.1) * 0.4 + 0.6) * 250) & 255));

          boolean[] ex = new boolean[4];
          int i = 0;

          for (int v = -1; v < 2; ++v)
            for (int u = -1; u < 2; ++u)
            {
              if (abs(u) != abs(v))
              {
                if ((x + u) >= 0 && (y + v) >= 0 &&
                  (x + u) < w && (y + v) < h)
                {
                  ex[i] = abs(occupied[(x + u) + (y + v) * w] - id) == 1;
                }
                ++i;
              }
            }

          int x0 = (x * width) / w + (ex[1] ? 0 : m);
          int y0 = (y * height) / h + (ex[0] ? 0 : m);
          int x1 = ((x + 1) * width) / w + (ex[2] ? 0 : -m);
          int y1 = ((y + 1) * height) / h + (ex[3] ? 0 : -m);

          rect(x0, y0, x1 - x0, y1 - y0);
        }
      }
  }
*/

  void update()
  {
    bool any_active = false;
    for (SnakeCollection::iterator s = snakes.begin(); s != snakes.end(); ++s)
    {
      if (s->active)
      {
        any_active = true;

        occupied[s->pos[0] + s->pos[1] * w] = (s->id << 16) + s->step_num;
        blackgrid[s->pos[0] + s->pos[1] * w] = s->black;

        Coord np = s->nextPos();

        if (clear(np[0], np[1]))
        {
          s->pos[0] = np[0];
          s->pos[1] = np[1];
          s->step_num += s->going_backwards ? -1 : +1;

          //if(s.turn_num > 10)
            //s.active = false;

          if((s->step_num % 20) == 0)
            s->dir = getFreeDirection(s->pos[0], s->pos[1], s->dir);
        }
        else
        {
          int nd = getFreeDirection(s->pos[0], s->pos[1], s->dir);

          if (nd == 4)
          {
            if(s->going_backwards)
              s->active = false;
            else
            {
              nd = getFreeDirection(s->start_pos[0], s->start_pos[1], -1);

              if(nd == 4)
                s->active = false;
              else
              {
                s->pos[0] = s->start_pos[0];
                s->pos[1] = s->start_pos[1];
                s->going_backwards = true;
                s->step_num = 32768;
                s->dir = nd;
              }
            }
          }
          else
          {
            s->dir = nd;
            s->turn_num++;
          }
        }
      }
    }

/*
    if (!any_active && restarts < 40)
    {
      List<int[]> empties = new ArrayList<int[]>();

      for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
          if (occupied[x + y * w] == 0)
          empties.add(new int[] {
            x, y
          }
          );
        }

      Collections.shuffle(empties);

      for (int i = 0; i < min(4, empties.size()); ++i)
      {
        Snake s = createSnake(empties.get(i)[0], empties.get(i)[1]);
        snakes.add(s);
      }

      ++restarts;
    }
    */
  }


  int getFreeDirection(int x, int y, int avoid_dir)
  {
    std::vector<int> dirs;;
    int nd = 0;

    for (int v = -1; v < 2; ++v)
      for (int u = -1; u < 2; ++u)
      {
        if (abs(u) != abs(v))
        {
          if (nd != avoid_dir && clear(x + u, y + v))
            dirs.push_back(nd);

          nd += 1;
        }
      }

    if(dirs.size() == 0)
      return 4;

    std::random_shuffle(dirs.begin(), dirs.end());

    return dirs[0];
  }

  int w, h, snake_id_counter;
  SnakeCollection snakes;
  int* occupied;
  bool* blackgrid;
  int restarts;
};



