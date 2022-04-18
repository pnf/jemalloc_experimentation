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
#include <atomic>
#include <jemalloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

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

     std::atomic_int tids(0);

     __thread extent_hooks_t *old_hooks;
     __thread int tid;
     int called = 0;

     __thread std::atomic_long *sleepMs;

}

extern "C" {

 void je_malloc_printf(const char *format, ...);

 /*

1. get arena for thread
2. get old hooks for thread
3.

 */

 void * my_extent_alloc(extent_hooks_t *extent_hooks, void *new_addr, size_t size,
     size_t alignment, bool *zero, bool *commit, unsigned arena_ind) {
     static __thread bool recurse = false;
     called++;
     if(!recurse) {
       recurse = true;
       std::cout << "extent_alloc tid=" << tid << " sz=" << size << " called=" << called << std::endl;

       long s;
       while ((s = sleepMs->load()) > 0) {
         std::cout << "tid=" << tid << " sleeping for " << s << std::endl;
         usleep(s * 1000);
       }
     }
     void * ret = (old_hooks->alloc)(extent_hooks, new_addr, size, alignment, zero, commit, arena_ind);
     recurse = false;
     return ret;
   }
}

JNIEXPORT jlong JNICALL Java_jemalloctest_JemallocIntegration_mkConfig(JNIEnv *, jclass) {
   return (jlong) (new std::atomic_long(0));
}

JNIEXPORT jlong JNICALL Java_jemalloctest_JemallocIntegration_setup(JNIEnv *, jclass, jlong config) {
     tid = tids++;

     unsigned arena;
     size_t len = sizeof(arena);
    int  ret = je_mallctl("thread.arena", &arena, &len, NULL, 0);
     std::cout << ret << "Thread " << tid << " Current arena " << arena << std::endl;

     char arena_cmd[30];
     len = sizeof(old_hooks);
     snprintf(arena_cmd, 29, "arena.%d.extent_hooks", arena);
     ret = je_mallctl(arena_cmd, &old_hooks, &len, NULL, 0);
     std::cout << arena_cmd << " = " << old_hooks->alloc << std::endl;

     // create new hooks and assign to new arena
     extent_hooks_t new_hooks = *old_hooks;
     extent_hooks_t *new_hooks_ptr = &new_hooks;
     new_hooks.alloc = &my_extent_alloc;
     snprintf(arena_cmd, 29, "arena.%d.extent_hooks", arena);
     ret = je_mallctl(arena_cmd, NULL, NULL, &new_hooks_ptr, sizeof(new_hooks_ptr));
     std::cout << ret << " set new hooks for " << arena_cmd << std::endl;

     // create new sleep variable
     sleepMs = ((std::atomic_long*) config);
     sleepMs->store(0);

     return 0;
}

 JNIEXPORT void JNICALL Java_jemalloctest_JemallocIntegration_teardown(JNIEnv *, jclass) {
     unsigned arena;
     char arena_cmd[30];
     size_t len = sizeof(old_hooks);
     snprintf(arena_cmd, 29, "arena.%d.extent_hooks", arena);
     int ret = je_mallctl(arena_cmd, NULL, NULL, &old_hooks, sizeof(old_hooks));
     std::cout << ret << " set old hooks for " << arena_cmd << std::endl;
     return;
 }

 JNIEXPORT void JNICALL Java_jemalloctest_JemallocIntegration_setSleep(JNIEnv *, jclass, jlong t, jlong s) {
    ((std::atomic_long*) t)->store(s);
 }

  JNIEXPORT jlong JNICALL Java_jemalloctest_JemallocIntegration_alloc(JNIEnv *, jclass, jlong sz) {
     std::cout << "tid=" << tid << " Allocating " << sz << std::endl;
     return (jlong) je_malloc((size_t) sz);

  }
  JNIEXPORT void JNICALL Java_jemalloctest_JemallocIntegration_free(JNIEnv *, jclass, jlong ptr) {
   je_free((void*) ptr);
  }


JNIEXPORT void JNICALL Java_jemalloctest_JemallocIntegration_sayHello(JNIEnv *, jclass) {

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
