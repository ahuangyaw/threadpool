/***********************************************************
  > File Name: thread_pool.cpp
  > Author: huang
  > Mail: tao_hwang@qq.com
  > Created Time: 2023年10月05日 星期四 15时31分15秒
 *******************************************************/
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <stack>
#include <algorithm>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
using namespace std;

void f(int x, int y)
{
    printf("x = %d  y = %d\n", x, y);
}

class Task
{
public:
    template<typename FUNC_T, typename ...ARGS>
    Task(FUNC_T func, ARGS...args)
    {
        this->func = bind(func, forward<ARGS>(args)...);
    }
    void run()
    {
        this->func();
        return;
    }

private:
    function<void()> func;
};

class ThreadPool
{
public:
    ThreadPool(int n) : threads(n)
    {
        for(int i = 0; i < n; i++)
        {
            //开辟n个线程
            threads[i] = new thread(&ThreadPool::worker, this);
        }
        return;
     }
    template<typename FUNC_T, typename ...ARGS>
    void add_task(FUNC_T func, ARGS...args) 
    {
        Task *t = new Task(func, forward<ARGS>(args)...);//开辟一个计算任务
        unique_lock<mutex> lock(this->mutex_t1);//抢占互斥锁
        tasks.push(t);//添加新任务
        m_cond.notify_one();//向其他线程提交通知
        return ;
    }
    void stop() 
    {
        for (int i = 0; i < threads.size(); i++) 
        {
            //让n个线程停止
            this->add_task(&ThreadPool::stop_thread, this);
        }
        for (auto x : threads) 
        {
            //等待线程结束
            x->join();
        }
        return ;
    }
private:
    void worker() 
    {
        thread::id id = this_thread::get_id();
        running[id] = true;
        while (running[id]) 
        {
            // get task
            Task *t = get_task();
            // run task
            t->run();
            delete t;
        }
        return ;
    }
    void stop_thread() 
    {
        thread::id id = this_thread::get_id();
        unique_lock<mutex> lock(this->mutex_t2);//多数容器都是非线程安全，需要加互斥锁
        running[id] = false;
        return ;
    }
    Task *get_task() 
    {
        unique_lock<mutex> lock(this->mutex_t1);//想要取任务先抢占互斥锁
        while (tasks.empty()) {m_cond.wait(lock);}//当计算任务为空，释放并等待通知
        Task *t = tasks.front();
        tasks.pop();
        return t;
    }
    map<thread::id, bool> running;//每个线程当前状态
    vector<thread *> threads;//线程
    queue<Task *> tasks;//互斥锁
    mutex mutex_t1, mutex_t2;
    condition_variable m_cond;//条件锁
};

int is_prime(int x) 
{
    if (x <= 1) return 0;
    for (int i = 2; i * i <= x; i++) 
    {
        if (x % i == 0) return 0;
    }
    return 1;
}

void count_prime(int b, int e, int &ret) 
{
    ret = 0;
    for (int i = b; i <= e; i++)
    {
        ret += is_prime(i);
    }
    return ;
}

#define MAX_N 50000000


int main()
{
    //统计素数个数
    ThreadPool tp(5);
    int n = 10, ret[n], batch = MAX_N / n;
    for (int i = 0, j = 1; i < 10; i++, j += batch) 
    {
        tp.add_task(count_prime, j, j + batch - 1, ref(ret[i]));
    }
    tp.stop();
    int ans = 0;
    for (auto x : ret) ans += x;
    cout << ans << endl;

    return 0;
}
