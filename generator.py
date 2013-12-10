import jinja2, random, os, subprocess, os.path, threading, time

random.seed()

def merge(*args):
    a = dict(args[0])
    for b in args[1:]:
        a.update(b)
    return a

empty_clear = {
    'empty': "return ds->empty();",
    #'clear': "ds->clear(); return 0;",
}

stack_actions = {
    'pop': "int x; return ds->pop(x) ? x : -1;", 
    'push': "ds->push({value}); return 0;",
}

queue_actions = {
    'dequeue': "int x; return ds->dequeue(x) ? x : -1;", 
    'enqueue': "ds->enqueue({value}); return 0;",
}

set_actions = {
    'find': "return ds->find({key});",
    'insert': "ds->insert({key}); return 0;",
    'erase': "return ds->erase({key});",
}

boost_actions = {
    'dequeue': "int x; return ds->dequeue(&x) ? x : -1;",
    'enqueue': "ds->enqueue({value}); return 0;",
    'empty': "return ds->empty();",
}

crange_actions = {
    'insert': 'ds->add({key}, 1, (void*) 1); return 0;',
    'erase': 'ds->del({key}, 1); return 0;',
    'find': 'return ds->search({key}, 1, 0) != nullptr ? 1 : 0;'
}

data_structures = {
    "cds::container::TreiberStack<{GC}, int>": merge(stack_actions, empty_clear),
    "cds::container::BasketQueue<{GC}, int>": merge(queue_actions, empty_clear),       # found bug
    "cds::container::MoirQueue<{GC}, int>": merge(queue_actions, empty_clear),         # found bug
    "cds::container::MSQueue<{GC}, int>": merge(queue_actions, empty_clear),          # found bug
    "cds::container::OptimisticQueue<{GC}, int>": merge(queue_actions, empty_clear),   # found bug
    "cds::container::RWQueue<int>": merge(queue_actions, empty_clear),
    #"cds::container::VyukovMPMCCycleQueue<int>": merge(queue_actions, empty_clear),    # does not compile
    "cds::container::LazyList<{GC}, int>": merge(set_actions, empty_clear),            # found bug
    "cds::container::SkipListSet<{GC}, int>": merge(set_actions, empty_clear),
    "boost::lockfree::fifo<int>": boost_actions,                                       # found bug
    "crange": crange_actions,
}

simple_data_structures = {
    'cds_treiberstack': "cds::container::TreiberStack<{GC}, int>",
    'cds_rwqueue': "cds::container::RWQueue<int>",
    'cds_skiplistset':  "cds::container::SkipListSet<{GC}, int>",
    'cds_basketqueue': "cds::container::BasketQueue<{GC}, int>",
    'cds_msqueue': "cds::container::MSQueue<{GC}, int>",
    'cds_optimisticqueue': "cds::container::OptimisticQueue<{GC}, int>",
    'cds_moirqueue': "cds::container::MoirQueue<{GC}, int>",
    'cds_lazylist': "cds::container::LazyList<{GC}, int>",
    'boost_fifo': "boost::lockfree::fifo<int>",
    'crange': 'crange',
}

environment = jinja2.Environment(line_statement_prefix='%%')

template = environment.from_string(open("generator-template.txt").read())

keys = [1]

def instantiate(line):
    if "{key}" in line:
        line = line.replace("{key}", str(random.choice(keys)))
    if "{value}" in line:
        line = line.replace("{value}", str(random.randint(0, 100000)))
    if "{GC}" in line:
        line = line.replace("{GC}", "cds::gc::HP")
    return line

def make_test_case(data_structure, num_threads, num_actions):
    data_structure = simple_data_structures[data_structure]
    threads = []
    for i in range(num_threads):
        thread = [instantiate(random.choice(data_structures[data_structure].values())) for j in range(num_actions)]
        threads.append(thread)

    return template.render({'data_structure': instantiate(data_structure), 'threads': threads, 'num_threads': len(threads)})

def make_general_case(data_structure, num_actions):
    data_structure = simple_data_structures[data_structure]
    threads = []
    for i in range(num_actions):
        for line in data_structures[data_structure].values():
            threads.append([instantiate(line)])
    return template.render({'data_structure': instantiate(data_structure), 'threads': threads, 'num_threads': len(threads)})

def make_specific_case(ds, threads):
    ds = simple_data_structures[ds]
    threads = [[instantiate(data_structures[ds][action]) for action in thread] for thread in threads]
    return template.render({'data_structure': instantiate(ds), 'threads': threads, 'num_threads': len(threads)})


#print make_test_case(random.choice(simple_data_structures.keys()), 4, 1)
#print make_general_case(random.choice(simple_data_structures.keys()), 2)

random.seed(0)
for ds in ["crange"]:
    for i in range(3):
        with open("cases/%s_rand_3x3_%d.cc" % (ds, i + 1), "w") as f:
            f.write(make_test_case(ds, 3, 3))

'''
bugs = [("cds_basketqueue", [["enqueue"], ["dequeue"], ["enqueue"], ["empty"], ["empty"]]),
    ("cds_msqueue", [["enqueue"], ["dequeue"], ["enqueue"], ["dequeue"]]),
    ("cds_optimisticqueue", [["enqueue"], ["enqueue"], ["dequeue"], ["empty"]]),
    ("cds_moirqueue", [["enqueue"], ["enqueue"], ["enqueue"], ["dequeue"], ["dequeue"]]),
    ("boost_fifo", [["enqueue"], ["dequeue"], ["enqueue"], ["enqueue"], ["empty"]]),
    ("cds_lazylist", [["find"], ["empty"], ["insert"], ["erase"]])]

for bug in bugs:
    ds, threads = bug
    with open("cases/%s_bug_%dx1.cc" % (ds, len(threads)), "w") as f:
        f.write(make_specific_case(ds, threads))
'''
