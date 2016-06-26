#include <list>
#include <cassert>
#include <string>

// static ctor (STM init)
extern std::string extStr("foo");

void test1()
{
    std::list<int> mylist;
    int myint;

    __transaction {
        mylist.push_back(5);
        mylist.push_back(7);
        mylist.pop_front();
    }

    assert (mylist.front() == 7);
    std::list<int>::iterator it = mylist.begin();

    __transaction {
      mylist.erase(it);
    }
}

int main()
{
    test1();
    return 0;
}
