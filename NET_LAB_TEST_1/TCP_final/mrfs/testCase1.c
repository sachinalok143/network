#include "mrfs.h"

using namespace std;

int main() {
    mrfs a;
    a.create_myfs(10);
    a.print();
    a.ls_myfs();
    cout<<"ls"<<endl;
    a.mkdir_myfs("hi");
    a.ls_myfs();
    cout<<"ls"<<endl;
    a.chdir_myfs("hi");
    a.ls_myfs();
    cout<<"ls"<<endl;
    a.chdir_myfs("..");
    a.ls_myfs();
    cout<<"ls"<<endl;
    a.dump_myfs("a.dmp");
    mrfs b;
    b.restore_myfs("a.dmp");
    b.print();
    b.chdir_myfs("hi");
    b.ls_myfs();
    b.chdir_myfs("hi");
    b.ls_myfs();
    b.chdir_myfs("..");
    b.ls_myfs();
    b.ls_myfs();
    cout<<"hi";
}