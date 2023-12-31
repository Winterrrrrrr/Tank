# 数据结构(H)大作业：坦克战队

# 项目介绍

## 项目要求

![image-20240109102636912](https://gitee.com/feng-zhiye/images/raw/master/imgs/image-20240109102636912.png)

## 游戏介绍

《[坦克战队](http://www.4399.com/flash/47110_1.htm)》是4399上一个类似英雄联盟/王者荣耀的攻塔守塔游戏。也是我小时候玩过的第一个这种类型的游戏。

游戏任务：指挥自己的坦克，配合己方作战单位，摧毁敌人防御塔和基地，获取胜利。

我花了两个多小时爬取了该游戏的贴图，用C++复现了该游戏的基本功能。

<img src="https://gitee.com/feng-zhiye/images/raw/master/imgs/image-20240109100529801.png" alt="image-20240109100529801" style="zoom:50%;" />



## 实现效果





## 特色

- 基本要求完全实现

- 扩充要求：
  - 图形化界面：基于SFML库
  - 多线程技术：基于线程池
  - 智能决策：两种索敌策略
  - 玩家控制角色：使用鼠标点击

# 前期准备：爬取flash动画

关于4399游戏flash动画的爬取可以参考B站这个UP主分享的视频：[Flash反编译swf文件 获取资源](https://www.bilibili.com/video/BV1wV411e7io/?vd_source=49d6aba24331143eae8d3fc696136e50)

需要使用搜狗/360浏览器。视频中用到的软件可以在[github](https://github.com/jindrapetrik/jpexs-decompiler)上下载。

注：有些游戏加密工作做得好，爬取不了。

<img src="https://gitee.com/feng-zhiye/images/raw/master/imgs/image-20240109103810180.png" alt="image-20240109103810180" style="zoom:50%;" />

# 基本框架：自顶向下

这个框架是我自己设计的，有比较强的个人风格。老师说一般游戏设计是采用MCV+线程模式，后续可以参考改进一下。

## Game类：一台状态机

<img src="https://gitee.com/feng-zhiye/images/raw/master/imgs/image-20240109102707815.png" alt="image-20240109102707815" style="zoom:50%;" />

- handleInput：处理输入
- update：根据新的输入和历史状态更新输出
- render：展示输出
- 单线程模式下，run()类会循环执行handleInput→update→render
- 采用多线程技术加持的“观察者模式”，可将update设置为一个在不断运行的独立子线程。而handleInput和render为主线程。

```cpp
void Game::run()
{
    threadPool.submit(gameUpdate, this);
    while(window.isOpen())
    {
        handleInput();
        render();
    }
}
```



## Screen类

![image-20240109102816599](https://gitee.com/feng-zhiye/images/raw/master/imgs/image-20240109102816599.png)

- 游戏在不同阶段应该有不同的界面，但具体操作逻辑不同。全部堆在一个Screen类里定义和实现会让代码相当臃肿。
- 因此我们可定义一个虚基类Screen，派生出三个子类。并在Game类中使用Screen指针实现在这三个子类中的切换。Game类状态机的操作都代理给screen类实现。

```cpp

void Game::update()
{
    sf::Clock clock;
	sf::Time timeSinceLastUpdate = sf::Time::Zero;

	while (window.isOpen())
	{
		sf::Time delta = clock.restart();
		timeSinceLastUpdate += delta;

		while (timeSinceLastUpdate > Game::TimePerFrame)
		{
			timeSinceLastUpdate -= TimePerFrame;
			Game::screen->update(TimePerFrame); //代理给screen类实现
		}
		
	}
	
}

```

### MenuScreen

菜单界面，可选择“开始”，“退出”，“操作说明”等。

选择“开始”时，game类里的Screen指针切换指向GameScreen类。

### GameScreen

游戏进行时的界面，在初始化时会生成地图，加载所有军事单位，并将其注册到Battlefield类中。

根据player的移动进行视角转移。

### GameOverScreen

游戏结束界面。



## MilitaryUnit

MilitaryUnit是一个虚基类，其他所有军事单位都继承该类。此外，游戏中的军事单位有四大行为：移动（move），旋转（rotate），索敌（detect）和攻击（attack）。此处我们充分利用**依赖倒置原则**，将这四大行为都代理给具体的行为类实现。

![image-20240109104243346](https://gitee.com/feng-zhiye/images/raw/master/imgs/image-20240109104243346.png)

## 四大行为

<img src="https://gitee.com/feng-zhiye/images/raw/master/imgs/image-20240109104433632.png" alt="image-20240109104433632" style="zoom: 50%;" />

### Move

```cpp
class Move
{
    public:
        Move(float velocity): velocity(velocity) {}
        virtual ~Move() = default;
        virtual void setRoute(const Route &route) = 0;
        virtual void move(MilitaryUnit &unit, sf::Vector2f &destination) = 0;
    protected:
        float velocity;
};

class PlayerMove: public Move
{
    public:
        PlayerMove(float velocity): Move(velocity) {}
        ~PlayerMove() = default;
        void setRoute(const Route &route) {}
        void move(MilitaryUnit &unit, sf::Vector2f &destination);
};

class SoldierMove: public Move
{
    public:
        SoldierMove(float velocity): Move(velocity) {}
        ~SoldierMove() = default;
        void setRoute(const Route &route);
        void move(MilitaryUnit &unit, sf::Vector2f &destination);

    private:
        Route route;
        void updateDest(const MilitaryUnit &unit, sf::Vector2f &destination);
    
};
```

#### Move的碰撞检测与碰撞处理

碰撞检测比较简单，获取两者几何中心的距离，和它们贴图的半径之和去比较就行。

```cpp

bool Battlefield::checkUnitCollison(const MilitaryUnit *unit1, const MilitaryUnit *unit2)
{
    //玩家和小兵可以重叠，不然太挤
    if(unit1->getType() == Type::player || unit2->getType() == Type::player) 
    {
        if(unit1->getType() == soldier || unit2->getType() == Type::soldier)
            return false;
    } 
    //同阵营小兵可以重叠，不然太挤
    if(unit1->getType() == soldier && unit2->getType() == Type::soldier && unit1->getSide() == unit2->getSide())
        return false;

    auto r1 = unit1->getRadius();
    auto r2 = unit2->getRadius();
    return getDistance(unit1, unit2) < r1 + r2;
}
```



对于碰撞处理，我一开始的代码逻辑是：撞到了就停止运动。

但这导致一个问题，即碰撞了以后连后退都不行。

因此需要更换处理方式，碰撞了以后不能停下来，而是往后退一小步。这一小步是肉眼看不出来的，但却可以让对象脱离碰撞状态，以便进行后退和转向。



```cpp
    if(Battlefield::checkCollision(&unit, destination))
    {
        float length=sqrt((destination.x-currentPos.x)*(destination.x-currentPos.x)
                    +(destination.y-currentPos.y)*(destination.y-currentPos.y));//必须先把double转成float
        if(length == 0) return;
        auto direction = sf::Vector2f(destination.x-currentPos.x , destination.y-currentPos.y) / length;
        auto xOffset = direction.x*velocity;
        auto yOffset = direction.y*velocity;
        currentPos.x -= xOffset; //退一步
        currentPos.y -= yOffset;
        sprite.setPosition(currentPos);
        destination = currentPos; //退完就别动了
        return;
    }
```

#### Move的最后一步

需要注意的是，由于我们采用图形化界面实现，游戏是一帧一帧更新的。这意味着游戏中的单位是离散地进行运动的，每帧运动固定的步长`offset`。这就很可能出现一种情况：即对象和目的地的距离小于最后一步的步长时，会出现在目的地附近来回抖动的情况。

![image-20240109105235885](https://gitee.com/feng-zhiye/images/raw/master/imgs/image-20240109105235885.png)

解决方式很简单，再加一个判断逻辑就可以。当对象和目的地的距离小于步长时，直接移动到目的地。

### Rotate

```cpp
class Rotate
{
    public:
        Rotate(float omega): omega(omega) {}
        bool rotate(MilitaryUnit &unit, const sf::Vector2f &destination);
    protected:
        float omega;
};
```

所有对象都是那么转，没什么好展开的，此处不作赘述。

### Detect

```cpp

class Detect
{
    public:
        Detect(float FOV): FOV(FOV) {}
        virtual ~Detect() = default;
        virtual bool detect(MilitaryUnit *self, MilitaryUnit *&target) = 0;
    
    protected:
        float FOV;

};

class LockDetect: public Detect
{
    public:
        LockDetect(float FOV);
        ~LockDetect() = default;
        bool detect(MilitaryUnit *self, MilitaryUnit *&target);

    private:
        std::default_random_engine random;

};

class MinDetect: public Detect
{
    public:
        MinDetect(float FOV):Detect(FOV) {}
        ~MinDetect() = default;
        bool detect(MilitaryUnit *self, MilitaryUnit *&target);

        struct Enemy{
            MilitaryUnit *target;
            float        distance;

            Enemy(MilitaryUnit *target, float distance): target(target), distance(distance) {}
        };

        struct cmp{
            bool operator()(Enemy a, Enemy b){
                return a.distance > b.distance; 
            }
        };

};
```

#### MinDetect：空间就近原则

小兵采用的索敌策略。扫描在视野内的敌人，锁定距离自己最近的目标。采用**最小堆**实现。

```cpp

bool MinDetect::detect(MilitaryUnit *self, MilitaryUnit *&target)
{
    auto units = Battlefield::getUnits();

    std::priority_queue<Enemy, std::vector<Enemy>, cmp> enemiesInVision;

    for (auto i : units)
    {
        if(i->isDead()) 
            continue;
        if(i->getSide() == self->getSide())  //大水淹了龙王庙
            continue;
        auto distance = Battlefield::getDistance(self, i);
        if(distance < FOV) //检测到了敌人
            enemiesInVision.push(Enemy(i,distance));
    }

    if(enemiesInVision.empty()) 
    {
        target = NULL;
        return false;
    }
    
    target = enemiesInVision.top().target;
    return true;
}


```



#### LockDetect：时间就近原则

防御塔采用的索敌策略。扫描在视野内的敌人，随机选择一个目标（优先小兵）。只要该目标还在视野内，就不改变。直至该目标消失在视野中，再重新选择。

```cpp
bool LockDetect::detect(MilitaryUnit *self, MilitaryUnit *&target)
{
    if(self->isDead()) return false;
    Units& units = Battlefield::getUnits();


    if((target != NULL) && (!target->isDead()) && (Battlefield::getDistance(self, target) < FOV)) //目标还在视野范围内
        return true;

    
    Units enemiesInVision;
    enemiesInVision.clear();
    for (auto i : units)
    {
        if(i->isDead()) 
            continue;
        if(i->getSide() == self->getSide())  //大水淹了龙王庙
            continue;
        if(Battlefield::getDistance(self, i) < FOV) //检测到了敌人
            enemiesInVision.push_back(i);
    }


    if(enemiesInVision.empty()) 
    {
        target = NULL;
        return false;
    }
    else if(enemiesInVision.size() == 1)
        target = enemiesInVision[0];
    else
    {

        std::uniform_int_distribution<int> distribution(0, enemiesInVision.size()-1); //int随机数范围是闭区间，最后得-1
        target = enemiesInVision[distribution(random)];
            
    }

    return true;
    

}
```



### Attack

```cpp
class Attack
{
    public:
        Attack(ShellSize size, int ATK, float attackRange, sf::Time attackInterval);
        bool attack(MilitaryUnit &attacker, sf::Time delta);
        void fire(const MilitaryUnit &attacker, MilitaryUnit &unit);

    private:
        ShellSize size; //炮弹类型
        float ATK; //攻击力
        float attackRange; //攻击距离
        sf::Time attackInterval; //攻击间隔（攻速）
        sf::Time attackClock; //用于判断是否满足攻击间隔

};
```

#### 炮弹

```cpp
enum ShellSize
{
    large,
    medium,
    small
};

class Shell
{
    public:
        Shell(ShellSize size, const MilitaryUnit &attacker, MilitaryUnit *target, int ATK);
        void update();
        bool isOver(); //完成轰炸任务
        bool attack(); //冲！
        void hurt(); //造成伤害
        void render(sf::RenderWindow& window);

    private:
        sf::Sprite sprite;
        MilitaryUnit *target; //攻击目标
        int damage; //预期造成的伤害
        bool hit; //击中标志
};
```



#### 攻击方式

```cpp

bool Attack::attack(MilitaryUnit &attacker, sf::Time delta)
{
    auto target = attacker.target;
    attackClock += delta;

    if(target == NULL || target->isDead()) return false;

    if(Battlefield::getDistance(&attacker, target) > attackRange) //超出攻击范围，先凑过去再说
    {
        attacker.moveDest = target->getPos();
        return false;
    }


    if(attackClock > attackInterval) //满足攻击间隔要求，进行下一次fire
    {
        attackClock = sf::Time::Zero;
        fire(attacker, *target);
        target = NULL;
        attacker.moveDest = attacker.getPos();
        return true;
    }
    
    else
    {
        attacker.moveDest = attacker.getPos();
        return false;
    }


}

void Attack::fire(const MilitaryUnit &attacker, MilitaryUnit &unit)
{
    auto shell = new Shell(size, attacker, &unit, ATK); //每次new一个炮弹来发射
    Battlefield::registerShell(shell);
    
}
```



### Battlefield

![image-20240109110422922](https://gitee.com/feng-zhiye/images/raw/master/imgs/image-20240109110422922.png)

- 每个军事单位都要和战场上的其他单位频繁地进行交互。如移动时要进行碰撞检测，看会不会撞到别人；攻击时要判断目标是不是在攻击范围内。这就要求每一个对象要频繁地对战场上的军事单位进行遍历判断。但我们不可能要求每一个对象自己内部存有场上其他对象的信息。因此我们可以设计一个谁都可以访问的**公共容器**来记录当前场上的所有的对象信息，以便随时读取遍历。
- 当Screen类生成并指向GameScreen类时，游戏开始。此时GameScreen类会初始化并加载游戏开始时的所有军事单位，并将这些单位注册到Battlefield类中。此外，基地每次生成的新士兵也会自动注册到Battlefield类中。有了该容器，我们可以**对场上所有军事单位进行统一的内存管理、更新维护和距离统计。**



## 线程池

有关线程池的部分我是直接参考[基于C++实现线程池](https://zhuanlan.zhihu.com/p/367309864)这个帖子实现的。关键就是实现一个任务队列和工作线程队列，让该类自动分配线程去执行任务队列里的任务。

<img src="https://gitee.com/feng-zhiye/images/raw/master/imgs/image-20240109110652451.png" alt="image-20240109110652451" style="zoom:67%;" />

### 线程排布

![image-20240109110709602](https://gitee.com/feng-zhiye/images/raw/master/imgs/image-20240109110709602.png)

- 整个程序一共有三个大的线程：update，handleInput和generateSoldiers。三个线程各自不断循环。

- 前两个线程是观察者模式中的“布告板”和“气象站”。第三个线程是生产小兵，因为该任务只是周期性地生产小兵，与游戏其他部分无关，因此也设置为一个独立线程。

- 在update中，Battlefiled类会并发地开启每一个对象的update，又产生诸多子线程。

```cpp
    ......
	std::vector<std::future<void>> unitFuture;
    std::vector<std::future<void>> shellFuture;

    for(auto unit: instance->units)
    {
        if(unit->isDead())
        {
            if(unit->getType() == Type::nexus)
            {
                instance->gameOver = true;
                if(unit->getSide() == Blue)
                    instance->winner = Side::Red;
                else
                    instance->winner = Side::Blue;
            }
            continue;
        }
        unitFuture.push_back(threadPool.submit([=]{unit->update(delta);}));
    }

    for(auto shell: instance->shells)
    {
        if(shell->isOver()) continue;
        shellFuture.push_back(threadPool.submit([=]{shell->update();}));
    }
    

    for(size_t i = 0; i < unitFuture.size(); i++)
        unitFuture[i].get();
    for(size_t i = 0; i < shellFuture.size(); i++)
        shellFuture[i].get();
......
```

# 其他技术细节

## Camera（卷轴模式）

游戏的窗口往往比游戏地图要小，因此我们需要移动窗口的位置，让窗口跟着玩家的视野移动。

SFML库的window有一个view参数，可以方便地设置窗口所处的位置。

```cpp
void Game::render()
{
	window.clear();
	Game::screen->render(window,view);
    window.setView(view);
	window.display();
}
```

此处我们让窗口随着玩家在y轴上的距离移动。

```cpp
void GameScreen::render(sf::RenderWindow& window, sf::View &view)
{
	backGround.draw(window);
    
    if(player->isDead())
        view.setCenter(sf::Vector2f(Parameter::windowWidth/2,Parameter::windowHeight/2));
    else
    {
        if(player->getPos().y < Parameter::windowHeight/2 || 
            player->getPos().y > Parameter::mapHeight - Parameter::windowHeight/2)
            view.setCenter(sf::Vector2f(Parameter::windowWidth/2, view.getCenter().y));
        else 
            view.setCenter(sf::Vector2f(Parameter::windowWidth/2, player->getPos().y));
    }

......
}
```



## 四大行为的配合

上文提到，每一个对象都可以执行四种行为，那么这四种行为的先后顺序应该如何规定呢？

此处我定了四条原则：

- 所有单位，在移动时应原地转向，直至头朝向目的地，才能进行移动。
- 所有单位，在将炮筒朝向目标前，不可攻击。
- 所有单位，在攻击时不可移动。
- 侦察与其他三个行为看似独立，但它会判断下一步该做什么。

将其翻译为代码如下：

```cpp
void Soldier::update(sf::Time delta)
{
    if(isDead()) return;
    detect();
    if(!rotate())
    {
        if(!attack(delta))
            move();
    }     
}
```

## 资源管理类：AssetManager

在生产小兵的时候由于要获取贴图，需要频繁地访问外存，这极大降低了代码的运行效率。为了解决这个问题，我又额外设计了一个资源管理类来管理资源。某资源一旦加载过，就存在内存里，无须再到外存去取。

```cpp
class AssetManager
{
    public:
        AssetManager();
        static sf::Texture& getTexture(const std::string &filename);
        static sf::SoundBuffer& getSoundbuffer(const std::string &filename);
        static sf::Font& getFont(const std::string &filename);
        
    private:
        std::map<std::string, sf::Texture> loadedTextures;
        std::map<std::string, sf::SoundBuffer> loadedSbuffers;
        std::map<std::string, sf::Font> loadedFonts;
        static AssetManager* instance;

};
```



```cpp

sf::Texture& AssetManager::getTexture(const std::string &filename) 
{
    auto& loadedTextures=instance->loadedTextures;
    auto check=loadedTextures.find(filename);
    if(check!=loadedTextures.end()) //若该资源已读取，则直接返回已读取的texture
    {
        return (*check).second;
    }
    else //若该资源未读取，则新建一个
    {
        auto& texture=loadedTextures[filename];
        if(!texture.loadFromFile(filename)) std::cout<<"fail to open"<<filename<<std::endl;
       // assert(texture.loadFromFile(filename)); 
        return texture;
    }
    
}
```



以生成小兵为例，我们在加载资源时需要调用如下语句：

```cpp
leftSoldiers[0].setTexture(AssetManager::getTexture("./pictures/redSoldier2.png"));
```



# 后续拓展

这是一些后续可以努力的方向。不过这只是个大作业，验收完代码就废了。估计以后我也不会再碰它（不是

![image-20240109112111736](https://gitee.com/feng-zhiye/images/raw/master/imgs/image-20240109112111736.png)
