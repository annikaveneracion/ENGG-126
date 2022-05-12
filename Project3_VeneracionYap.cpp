// ENGG 126 Project 3 - Veneracion & Yap 

#include <unistd.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>

using namespace std;

constexpr int P = 5;
thread philosophers[P];
mutex mtx[P]; //forks
mutex coutmtx;
int ate[5] = {0}; // Counting which philosopher ate how many times
int runs = 100; 

void think(int id) 
{
    this_thread::sleep_for(chrono::milliseconds(500)); //to simulate thinking time
    coutmtx.lock();
    cout << "Philosopher " << to_string(id) << " is thinking." << endl;
    coutmtx.unlock();
}

bool eat(int id, int left, int right)
{
    while (1)
    if (mtx[left].try_lock()) 
    { //try to get left fork
        if (mtx[right].try_lock()) 
        { // try to get right fork
            coutmtx.lock();
            
            cout << "Philosopher " << to_string(id) << " got the fork " << to_string(right) << 
            "\nPhilosopher " << to_string(id) << " eats." << endl; 
            coutmtx.unlock(); 

            return true; // only eat if both forks are picked up
        }
        else 
        {
            mtx[left].unlock(); // dont eat since right fork is not available
        }
    }
    return false;
}

void putDown(int left, int right, int id) 
{
    mtx[left].unlock(); // Put down the left fork
    mtx[right].unlock(); // Put down the right fork

    coutmtx.lock();
    cout << "Philosopher " << to_string(id) << " is done." << endl; 
    coutmtx.unlock(); 
}

void dinnerStart(int id) 
{
    int ids = (id + 1) % P;
    int lIndex = min(id, ids);
    int rIndex = max(id, ids);

    while (runs-- > 0) // decrease interrupt after each time all philosophers eat
                            // to simulate the code for n times 
    { 
        think(id);
        if (eat(id, lIndex, rIndex)) 
        {
            putDown(lIndex, rIndex, id); // Put down the forks
            ate[id]++; 
            this_thread::sleep_for(chrono::milliseconds(600)); //cannot acquire eating right away
        }
    }

}

void dine(){
	
    for (int i = 0; i < P; ++i)
    { 
        philosophers[i] = thread(dinnerStart, i);
    }

	for (int i = 0; i < P; ++i) 
    {
        philosophers[i].join(); // join threads after dinnerStart
    }
}

int main() { 
    int j;
    cout << "Press any number to check on philosophers: ";
    cin >> j;
    cout << endl;

    if (j >= 0)
    {
        dine(); 
        for (int i = 0; i < P; ++i)
        {
            cout << i << " = " << ate[i] << endl;
        }
    }
}