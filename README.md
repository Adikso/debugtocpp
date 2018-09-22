# debugtocpp

A tool that allows to generate C++ style headers or JSON from both PDB and DWARF/ELF debug formats. It is also focused on generating classes with pointers to members so they can be called in injected libraries (e.g modding games).

**Tool is still in development and IS NOT perfectly accurate (it is bugged)**

## Installation

```
git clone --recurse-submodules https://github.com/Adikso/debugtocpp && cd debugtocpp
mkdir build && cd build
cmake .. && make
./debugtocpp
```

## Examples
### DWARF
Original source:
```cpp
class Test : public Atest{
public:
    bool funnyBool;
    static int funnyStatic;
    int array[4] = {2, 1, 3, 7};
    int * ptrArray = new int[4]{2, 1, 3, 7};

    int * funnyFunc(int someArg);
    const int funnyFunc(int &someArg);

private:
    bool karamba;
};
```

Extracted:
```cpp
class Test : public Atest {
public:
  bool funnyBool;
  static int funnyStatic;
  int array[];
  int * ptrArray;
private:
  bool karamba;

public:
  int * funnyFunc(int someArg);
  const int funnyFunc(int& someArg);
};
```

### PDB
Original source:
```cpp
class c_ArmoredSkeleton : public c_Enemy{
public:
    bool m_hasHead;
    c_Point* m_cachedMoveDir;
    int m_directionHitFrom;
    bool m_justBounced;
    int m_shieldDir;
    bool m_shieldDestroyed;
    bool m_gotBounced;

    c_ArmoredSkeleton();
    c_ArmoredSkeleton* m_new(int t_xVal,int t_yVal,int t_l);
    c_ArmoredSkeleton* m_new2();
    bool p_CanBeLord();
    void p_Die();
    c_Point* p_GetMovementDirection();
    bool p_Hit(String t_damageSource,int t_damage,int t_dir,c_Entity* t_hitter,bool t_hitAtLastTile,int t_hitType);
    void p_MoveFail();
    int p_MoveImmediate(int t_xVal,int t_yVal,String t_movementSource);
    void p_AdjustShieldDir();
    void p_MoveSucceed(bool t_hitPlayer,bool t_moveDelayed);
    void p_Update();
    void mark();
};
```

Extracted:
```cpp
class c_ArmoredSkeleton : public c_Enemy {
public:
  bool m_hasHead;
  c_Point * m_cachedMoveDir;
  int m_directionHitFrom;
  bool m_justBounced;
  int m_shieldDir;
  bool m_shieldDestroyed;
  bool m_gotBounced;

  c_ArmoredSkeleton * m_new(int t_xVal, int t_yVal, int t_l);
  c_ArmoredSkeleton * m_new2();
  virtual bool p_CanBeLord();
  virtual void p_Die();
  virtual c_Point * p_GetMovementDirection();
  virtual bool p_Hit(String * t_damageSource, int t_damage, int t_dir, c_Entity * t_hitter, bool t_hitAtLastTile, int t_hitType);
  virtual void p_MoveFail();
  virtual int p_MoveImmediate(int t_xVal, int t_yVal, String * t_movementSource);
  void p_AdjustShieldDir();
  virtual void p_MoveSucceed(bool t_hitPlayer, bool t_moveDelayed);
  virtual void p_Update();
  virtual void mark();
  virtual ~c_ArmoredSkeleton();
  void __local_vftable_ctor_closure();
  void * __vecDelDtor(unsigned int);
};

```

### ELF
Original source:
```cpp
class BigCoin : public Enemy {
public:
    static int * vtable;

    static void Define();
    static BigCoin * Construct();

    static BigCoin * New(BigCoin * mob, int x, int y, int type);
    static bool Hit(BigCoin * mob, String * t_damageSource,int t_damage,int t_dir, Entity* t_hitter,bool t_hitAtLastTile,int t_hitType);

    static void MoveSucceed(BigCoin * mob, bool hitPlayer, bool moveDelayed);
    static int AttemptMove(BigCoin * mob, int x, int y);

    static void Update(BigCoin * mob);
};
```

Extracted:
```cpp
class BigCoin {
public:
  static int vtable;

  int AttemptMove(BigCoin * arg1, int arg2, int arg3);
  int New(BigCoin * arg1, int arg2, int arg3, int arg4);
  int Construct();
  int Define();
  int Hit(BigCoin * arg1, String * arg2, int arg3, int arg4, Entity * arg5, bool arg6, int arg7);
  int MoveSucceed(BigCoin * arg1, bool arg2, bool arg3);
  int Update(BigCoin * arg1);
};
```