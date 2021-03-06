/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// This tests a Gr class
#if SK_SUPPORT_GPU

#include "GrMemoryPool.h"
#include "SkBenchmark.h"
#include "SkRandom.h"
#include "SkTScopedPtr.h"
#include "SkTDArray.h"

// change this to 0 to compare GrMemoryPool to default new / delete
#define OVERRIDE_NEW    1

namespace {
struct A {
    int gStuff[10];
#if OVERRIDE_NEW
    void* operator new (size_t size) { return gPool.allocate(size); }
    void operator delete (void* mem) { if (mem) { return gPool.release(mem); } }
#endif
    static GrMemoryPool gPool;
};
GrMemoryPool A::gPool(10 * (1 << 10), 10 * (1 << 10));
}


/**
 * This benchmark creates and deletes objects in stack order
 */
class GrMemoryPoolBenchStack : public SkBenchmark {
    enum {
        N = SkBENCHLOOP(1 * (1 << 20)),
    };
public:
    GrMemoryPoolBenchStack(void* param) : INHERITED(param) {
        fIsRendering = false;
    }
protected:
    virtual const char* onGetName() {
        return "grmemorypool_stack";
    }

    virtual void onDraw(SkCanvas*) {
        SkRandom r;
        enum {
            kMaxObjects = 4 * (1 << 10),
        };
        A* objects[kMaxObjects];

        // We delete if a random [-1, 1] fixed pt is < the thresh. Otherwise,
        // we allocate. We start allocate-biased and ping-pong to delete-biased
        SkFixed delThresh = -SK_FixedHalf;
        enum {
            kSwitchThreshPeriod = N / (2 * kMaxObjects),
        };
        int s = 0;

        int count = 0;
        for (int i = 0; i < N; i++, ++s) {
            if (kSwitchThreshPeriod == s) {
                delThresh = -delThresh;
                s = 0;
            }
            SkFixed del = r.nextSFixed1();
            if (count &&
                (kMaxObjects == count || del < delThresh)) {
                delete objects[count-1];
                --count;
            } else {
                objects[count] = new A;
                ++count;
            }
        }
        for (int i = 0; i < count; ++i) {
            delete objects[i];
        }
    }

private:
    typedef SkBenchmark INHERITED;
};

/**
 * This benchmark creates objects and deletes them in random order
 */
class GrMemoryPoolBenchRandom : public SkBenchmark {
    enum {
        N = SkBENCHLOOP(1 * (1 << 20)),
    };
public:
    GrMemoryPoolBenchRandom(void* param) : INHERITED(param) {
        fIsRendering = false;
    }
protected:
    virtual const char* onGetName() {
        return "grmemorypool_random";
    }

    virtual void onDraw(SkCanvas*) {
        SkRandom r;
        enum {
            kMaxObjects = 4 * (1 << 10),
        };
        SkTScopedPtr<A> objects[kMaxObjects];

        for (int i = 0; i < N; i++) {
            uint32_t idx = r.nextRangeU(0, kMaxObjects-1);
            if (NULL == objects[idx].get()) {
                objects[idx].reset(new A);
            } else {
                objects[idx].reset(NULL);
            }
        }
    }

private:
    typedef SkBenchmark INHERITED;
};

/**
 * This benchmark creates objects and deletes them in queue order
 */
class GrMemoryPoolBenchQueue : public SkBenchmark {
    enum {
        N = SkBENCHLOOP((1 << 8)),
        M = SkBENCHLOOP(4 * (1 << 10)),
    };
public:
    GrMemoryPoolBenchQueue(void* param) : INHERITED(param) {
        fIsRendering = false;
    }
protected:
    virtual const char* onGetName() {
        return "grmemorypool_queue";
    }

    virtual void onDraw(SkCanvas*) {
        SkRandom r;
        A* objects[M];
        for (int i = 0; i < N; i++) {
            uint32_t count = r.nextRangeU(0, M-1);
            for (uint32_t i = 0; i < count; i++) {
                objects[i] = new A;
            }
            for (uint32_t i = 0; i < count; i++) {
                delete objects[i];
            }
        }
    }

private:
    typedef SkBenchmark INHERITED;
};

///////////////////////////////////////////////////////////////////////////////

static SkBenchmark* Fact1(void* p) { return new GrMemoryPoolBenchStack(p); }
static SkBenchmark* Fact2(void* p) { return new GrMemoryPoolBenchRandom(p); }
static SkBenchmark* Fact3(void* p) { return new GrMemoryPoolBenchQueue(p); }

static BenchRegistry gReg01(Fact1);
static BenchRegistry gReg02(Fact2);
static BenchRegistry gReg03(Fact3);

#endif
