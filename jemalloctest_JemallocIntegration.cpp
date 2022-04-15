/*

On macos, build jemalloc with explicit rpath
  ./configure --prefix=/Users/pnf/dev/jemalloc/install
(otherwise, it will search in /usr/lib at runtime)

JAVA_HOME=/Users/pnf/Library/Java/JavaVirtualMachines/azul-11.0.13/Contents/Home
JEMALLOC=/Users/pnf/dev/jemalloc/install

To extract .h
$JAVA_HOME/bin/javac -h . src/main/scala/jemalloctest/JemallocIntegration.java
(then delete the .class file)

g++ -c -fPIC -I$JEMALLOC/include -I${JAVA_HOME}/include -I${JAVA_HOME}/include/darwin jemalloctest_JemallocIntegration.cpp -o jemalloctest_JemallocIntegration.o

g++ -L$JEMALLOC/lib -ljemalloc -dynamiclib -dynamiclib -o libnative.dylib jemalloctest_JemallocIntegration.o

*/
//  ~/Library/Java/JavaVirtualMachines/openjdk-16.0.2/Contents/Home/
// JAVA_HOME=

#include "jemalloctest_JemallocIntegration.h"
#include <iostream>
#include <jemalloc.h>
#include <stdlib.h>

/*
extern "C" {
int je_mallctl(const char *name,
 	void *oldp,
 	size_t *oldlenp,
 	void *newp,
 	size_t newlen);
}
*/

namespace {
     extent_hooks_t *old_hooks;
     int called = 0;
}

extern "C" {
 void * my_extent_alloc(extent_hooks_t *extent_hooks, void *new_addr, size_t size,
     size_t alignment, bool *zero, bool *commit, unsigned arena_ind) {
     static bool recurse = false;
     called++;

     if(!recurse) {
       recurse = true;
       std::cout << "extent_alloc size=" << size << " called=" << called << std:: endl;
     }
     void * ret = (old_hooks->alloc)(extent_hooks, new_addr, size, alignment, zero, commit, arena_ind);
     recurse = false;
     return ret;
   }
}

JNIEXPORT void JNICALL Java_jemalloctest_JemallocIntegration_sayHello(JNIEnv *, jobject) {

     int ret;

     std::cout << "Hello from C++ !!" << std::endl;
     const char *v;
     size_t len = sizeof(v);
     je_mallctl("version", &v, &len,	NULL,  	0);
     std::cout << ret << " " << len << " version "<< v << std::endl;

     // Create new arena
     unsigned arena;
     len = sizeof(arena);
     ret =     je_mallctl("arenas.create", &arena, &len, NULL, 0);
     std::cout << ret << " Created new arena " << arena << std::endl;

     unsigned old_arena;
     len = sizeof(old_arena);
     ret = je_mallctl("thread.arena", &old_arena, &len, NULL, 0);
     std::cout << ret << " Current arena " << old_arena << std::endl;

     // get extent hooks
     char arena_cmd[30];
     len = sizeof(old_hooks);
     snprintf(arena_cmd, 29, "arena.%d.extent_hooks", old_arena);
     ret = je_mallctl(arena_cmd, &old_hooks, &len, NULL, 0);
     std::cout << arena_cmd << " = " << old_hooks->alloc << std::endl;

     // create new hooks and assign to new arena
     extent_hooks_t new_hooks = *old_hooks;
     extent_hooks_t *new_hooks_ptr = &new_hooks;
     new_hooks.alloc = &my_extent_alloc;
     snprintf(arena_cmd, 29, "arena.%d.extent_hooks", arena);
     ret = je_mallctl(arena_cmd, NULL, NULL, &new_hooks_ptr, sizeof(new_hooks_ptr));
     std::cout << ret << " set new hooks for " << arena_cmd << std::endl;

     std::cout << "Called " << called << " times" << std::endl;

     //Set new arena to current thread
     ret = je_mallctl("thread.arena", NULL, NULL, &arena, sizeof(arena));
     std::cout << ret << " set current thread to new arena" << std::endl;

     for(int i = 0; i < 100; i++) {
       size_t sz = 1 << (rand() % 20);
       je_malloc(sz);
       }

     std::cout << "Called " << called << " times" << std::endl;

     // Restore old arena
     //Set new arena to current thread
     ret = je_mallctl("thread.arena", NULL, NULL, &old_arena, sizeof(arena));
     std::cout << ret << " set current thread to old arena" << std::endl;

    return;
}
