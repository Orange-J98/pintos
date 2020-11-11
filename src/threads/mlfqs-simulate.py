load_avg: float = 0.0
tick: int = 0

PRI_MAX = 63
PRI_MIN = 0
TIMER_FREQ = 100

# checkpoints = [0, 4, 8, 12, 16, 20, 24, 28, 32, 36]


class Thread():
    nice: int
    recent_cpu: float = 0.0
    name: str
    priority: int = 0

    def __init__(self, nice: int, name: str) -> None:
        self.nice = nice
        self.name = name


threads = []
threads.append(Thread(nice=0, name="A"))
threads.append(Thread(nice=1, name="B"))
threads.append(Thread(nice=2, name="C"))


running_thread: Thread = None

rr_counter = 0

while tick < 40:
    # increase recent cpu for running thread
    if running_thread is not None:
        running_thread.recent_cpu += 1

    if tick % TIMER_FREQ == 0:
        # update load_avg
        ready_threads = len(threads)
        load_avg = (59/60)*load_avg + (1/60)*ready_threads

        # update recent_cpu of each thread
        for t in threads:
            t.recent_cpu = (2*load_avg)/(2*load_avg + 1) * \
                t.recent_cpu + t.nice

    # update priority
    for t in threads:
        t.priority = int(PRI_MAX - (t.recent_cpu / 4) - (t.nice * 2))

    if tick % 4 == 0:
        # scheduler
        max_pri = max([t.priority for t in threads])
        max_threads = list([t for t in threads if t.priority == max_pri])
        if len(max_threads) == 1:
            running_thread = max_threads[0]
        else:
            # round robin
            running_thread = max_threads[rr_counter % len(max_threads)]
            rr_counter += 1

        # print stats
        print(f"{tick:5d}  ", end='')
        for t in threads:
            print(f"{round(t.recent_cpu):2d}  ", end='')
        for t in threads:
            print(f"{int(t.priority):2d}  ", end='')
        print(f"   {running_thread.name}")

    tick += 1
