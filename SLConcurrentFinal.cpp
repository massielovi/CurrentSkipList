#include <iostream>
#include <random>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>

using namespace std;
int g_num = 0;

#define max_value 99999999
#define min_value -99999999
#define MAX_LEVEL 5

int get_random(int low, int high) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribution(low, high);
    return distribution(gen);
}

template <class T>
class nodo
{
public:
    //parallel
    mutex lock1;
    //mutex[] m_next;
    T valor;
    //nodo<T>**next;
    vector<nodo<T>*> next;

    bool deleted = false;
    bool linked = false;
    int top_level;

    //level es un random entre(0-max_levle)
    nodo(int v) {
        valor = v;
        next = new nodo <T>[top_level];
    }
    nodo(T v, int level) {
        valor = v;
        next.resize(level + 1);
        for (int i = 0; i < next.size(); i++) {
            next[i] = nullptr;
        }
        top_level = level;//que es lo mismo q el size
    }

    void lock() {
        lock1.lock();
    }
    void unlock() {
        lock1.unlock();
    }

};


template <class T>
class SL {

    int level_max;
    float P;//Probabilidad
    //nodo<T>* head;
     //new
    int max_level;
    nodo<T>* head;
    nodo<T>* tail;


public:

    SL()
    {
        max_level = MAX_LEVEL;
        head = new nodo<T>(min_value, max_level);
        tail = new nodo<T>(max_value, max_level);
        this->P = 0.5;
        //new
        for (int i = 0; i < head->next.size(); i++) {
            head->next[i] = tail;
        }
    }


    void print()
    {
        cout << "\n -----SKIP LIST-----  " << "\n";
        for (int i = 0; i <= max_level; i++)
        {
            nodo<T>* node = head->next[i];
            cout << "Level " << i << ":  ";
            while (node != tail)
            {
                cout << node->valor << " ";
                node = node->next[i];
            }
            cout << "\n";
        }
    }


    float randoom()
    {
        return (float)rand() / RAND_MAX;
    }


    int Choose_lvl()
    {
        float r = (float)rand() / RAND_MAX;
        int lvl = 0;
        while (r < P && lvl < max_level)
        {
            lvl++;
            r = (float)rand() / RAND_MAX;
        }
        return lvl;
    }


    //Parallel
    int Find_p(T x, vector<nodo<T>*>& preds, vector<nodo<T>*>& sucess) {
        int level_found = -1;
        nodo <T>* pred = head;

        for (int level = max_level; level >= 0; level--) {
            nodo <T>* curr = pred->next[level];
            while (curr->valor < x) {
                pred = curr; curr = pred->next[level];
            }
            if (level_found == -1 && curr->valor == x) {
                level_found = level;
            }
            preds[level] = pred;
            sucess[level] = curr;
        }
        return level_found;
    }


    bool Add_p(T x) {

        vector<nodo<T>*> preds(max_level + 1);
        vector<nodo<T>*> succs(max_level + 1);


        for (int i = 0; i < preds.size(); i++) {
            preds[i] = nullptr;
            succs[i] = nullptr;
        }

        int top_level = Choose_lvl();//dentro deberia esatr el MAX
        //cout << "INSERTANDO x: " << x << " NIVEL: " << top_level << endl;

        while (true) {
            int level_found = Find_p(x, preds, succs);//inicializamos 
            if (level_found != -1) {//ha sido encontrado
                nodo<T>* node_found = succs[level_found];
                if (node_found->deleted) {//no ha sido eliminado
                    while (!node_found->linked) {//no ha sido encontrado
                        return false;
                    }
                }
                continue;
            }


            //desde aqui es diferente
            int hightlocked = -1;

            nodo <T>* pred;
            nodo <T>* succ;
            nodo <T>* prevPred = NULL;
            bool valid = true;

            for (int level = 0; valid && (level <= top_level); level++) {
                pred = preds[level];
                succ = succs[level];

                if (pred != prevPred) {
                    pred->lock();
                    hightlocked = level;
                    prevPred = pred;
                }
                valid = !pred->deleted && !succ->deleted && pred->next[level] == succ;
            }


            if (!valid) {
                nodo <T>* tmp = nullptr;
                for (int level = 0; level <= hightlocked; level++)
                {
                    if (tmp == nullptr || tmp != preds[level])
                        preds[level]->unlock();
                    tmp = preds[level];
                }
                continue;
            }



            nodo<T>* newNode = new nodo<T>(x, top_level);
            for (int level = 0; level <= top_level; level++) {
                newNode->next[level] = succs[level];
                preds[level]->next[level] = newNode;
            }
            newNode->linked = true;

            nodo <T>* tmp = nullptr;
            for (int level = 0; level <= hightlocked; level++)
            {
                if (tmp == nullptr || tmp != preds[level])
                    preds[level]->unlock();
                tmp = preds[level];
            }
            return true;
        }
    }

    
    bool oktodelete(nodo<T>* candidate, int lFound) {
        return( candidate->linked && candidate->top_level == lFound && !candidate->deleted);
    }

    bool Del_p(T x) {

        nodo<T>* to_del = nullptr;

        bool isMarked = false;
        int top_level = -1;//dentro deberia esatr el MAX

        vector<nodo<T>*> preds(max_level + 1);
        vector<nodo<T>*> succs(max_level + 1);

        //cout << "ELIMINANDO x: " << x << endl;

        while (true) {
            int level_found = Find_p(x, preds, succs);//inicializamos 
            if (isMarked || (level_found != -1 && oktodelete(succs[level_found], level_found))) {//ha sido encontrado
                //nodo<T> node_found = sucess[level_found];
                if (!isMarked) {
                    to_del = succs[level_found];
                    top_level = to_del->top_level;
                    to_del->lock();
                    if (to_del->deleted) {//no ha sido eliminado
                        to_del->unlock();
                        return false;
                    }
                    to_del->deleted = true;
                    isMarked = true;
                }
                int hightlocked = -1;
                nodo <T>* pred;
                nodo <T>* succ;
                nodo <T>* prevPred = NULL;

                bool valid = true;

                for (int level = 0; valid && (level <= top_level); level++) {
                    pred = preds[level];
                    succ = succs[level];

                    if (pred != prevPred) {
                        pred->lock();
                        hightlocked = level;
                        prevPred = pred;
                    }
                    valid = !pred->deleted && pred->next[level] == succ;
                }
                if (!valid) {
                    nodo <T>* tmp = nullptr;

                    for (int level = 0; level <= hightlocked; level++) {
                        if (tmp == NULL || tmp != preds[level]) preds[level]->unlock();
                        tmp = preds[level];
                    }
                    continue;
                }
                //nodo <T> newNode= new nodo <T>(x,top_level);
                for (int level = 0; level <= top_level; level++) {
                    preds[level]->next[level] = to_del->next[level];
                }
                to_del->unlock();

                nodo <T>* tmp = NULL;
                for (int level = 0; level <= hightlocked; level++) {
                    if (tmp == NULL || tmp != preds[level]) preds[level]->unlock();
                    tmp = preds[level];
                }
                return true;
            }
            else return false;
        }

    };
};

template <class T>
struct add_x {
    SL <T>* ptrm_;
    //SL<T>* ptrm_;
    int min_, max_;

    add_x(int min, int max, SL<T>* ptrm) : min_(min), max_(max), ptrm_(ptrm) {}

    void operator()(int y) {
        for (int i = 0; i < y; i++) {
            x = get_random(min_, max_);
            //cout << "number to add: " << x << endl;
            ptrm_->Add_p(x);
        }
    };

private:
    int x;
};


template <class T>
struct del_x {
    SL <T>* ptrm_;
    int min_, max_;

    del_x(int min, int max, SL<T>* ptrm) : min_(min), max_(max), ptrm_(ptrm) {};

    void operator()(int y) {
        for (int i = 0; i < y; i++) {
            x = get_random(min_, max_);
            //cout << "number to delete: " << x << endl;
            ptrm_->Del_p(x);
        }
    };

private:
    int x;
};


int main()
{
    SL <int> A;
    vector<thread>A1;
    vector<thread>A2;

    std::chrono::time_point<std::chrono::system_clock> start, end;

    start = std::chrono::system_clock::now();

    for (int i = 0; i < 4; i++) {
        A1.push_back(thread(add_x<int>(1, 100, &A), 25));
        //A2.push_back(thread(add_x<int>(1, 100, &A), 25));
    }

    for (int i = 0; i < 4; i++) {
        A1[i].join();
       // A2[i].join();
    }

    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);

    std::cout << "finished computation at " << std::ctime(&end_time)
        << "elapsed time: " << elapsed_seconds.count() << "s\n";

    return 0;

    /*A.Add_p(6);
    A.Add_p(5);
    A.Add_p(7);
    A.Add_p(2);
    A.Add_p(9);

    A.print();

    A.Del_p(7);
    A.Del_p(2);
    A.Del_p(9);

    A.print();*/
}