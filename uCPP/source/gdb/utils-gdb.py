# 
# Copyright (C) Lynn Tran, Jiachen Zhang 2018
# 
# utils-gdb.py -- 
# 
# Author           : Lynn Tran
# Created On       : Mon Oct 1 22:06:09 2018
# Last Modified By : Peter A. Buhr
# Last Modified On : Thu Jun 20 07:27:23 2024
# Update Count     : 48
# 

"""
To run this extension, the python name has to be as same as one of the loaded library
Additionally, the file must exist in a folder which is in gdb's safe path
"""
import collections
import gdb
import re

# set these signal handlers with some settings (nostop, noprint, pass)
gdb.execute('handle SIGALRM nostop noprint pass')
gdb.execute('handle SIGUSR1 nostop noprint pass')

uCPPTypes = collections.namedtuple('uCPPTypes', 'uCluster_ptr_type \
        uClusterDL_ptr_type uBaseTask_ptr_type uBaseTaskDL_ptr_type int_ptr_type')

# A named tuple representing information about a stack
StackInfo = collections.namedtuple('StackInfo', 'sp fp pc')

# A global variable to keep track of stack information as one switches from one
# task to another task
STACK = []

# A global variable to keep all system task name
SysTask_Name = ["uLocalDebuggerReader", "uLocalDebugger", "uProcessorTask", "uBootTask", "uSystemTask", 
"uProcessorTask", "uPthread", "uProfiler"]

not_supported_error_msg = "Not a supported command for this language"

def get_uCPP_types():
    # GDB types for various structures/types in uC++
    return uCPPTypes(uCluster_ptr_type = gdb.lookup_type('uCluster').pointer(),
                     uClusterDL_ptr_type = gdb.lookup_type('uClusterDL').pointer(),
                     uBaseTask_ptr_type = gdb.lookup_type('uBaseTask').pointer(),
                     uBaseTaskDL_ptr_type = gdb.lookup_type('uBaseTaskDL').pointer(),
                     int_ptr_type = gdb.lookup_type('int').pointer())

def get_addr(addr):
    """
    NOTE: sketchy solution to retrieve address. There is a better solution...
    @addr: str of an address that can be in a format 0xfffff <type of the object
    at this address>
    Return: str of just the address
    """
    str_addr = str(addr)
    ending_addr_index = str_addr.find('<')
    if ending_addr_index == -1:
        return str(addr)
    return str_addr[:ending_addr_index].strip()

def print_usage(msg):
    """
    Print out usage message
    @msg: str
    """
    print('Usage: ' + msg)

def parse_argv_list(args):
    """
    Split the argument list in string format, where each argument is separated
    by whitespace delimiter, to a list of arguments like argv
    @args: str of arguments
    Return:
        [] if args is an empty string
        list if args is not empty
    """
    # parse the string format of arguments and return a list of arguments
    argv = args.split(' ')
    if len(argv) == 1 and argv[0] == '':
        return []
    return argv

def get_cluster_root():
    """
    Return: gdb.Value of globalClusters.root (is an address)
    """
    # globalClusters is uNoCtor<uClusterSeq, false> => explicitly cast raw storage on stack
    cluster_root = gdb.parse_and_eval('((uClusterSeq &)*((uClusterSeq *)&uKernelModule::globalClusters)).root')
    if cluster_root.address == 0x0:
        print('No clusters, program terminated')
    return cluster_root

def lookup_cluster_by_name(cluster_name):
    """
    Look up a cluster given its ID
    @cluster_name: str
    Return: gdb.Value
    """
    uCPPTypes = None
    try:
        uCPPTypes = get_uCPP_types()
    except gdb.error:
        print(not_supported_error_msg)
        return None

    cluster_root = get_cluster_root()
    if cluster_root.address == 0x0:
        return cluster_root.address

    # lookup for the task associated with the id
    cluster = 0x0
    curr = cluster_root
    while True:
        if curr['cluster_']['name'].string() == cluster_name:
            cluster = curr['cluster_'].address
            break
        curr = curr['next'].cast(uCPPTypes.uClusterDL_ptr_type)
        if curr == cluster_root:
            break

    if cluster == 0x0:
        print("Cannot find a cluster with the name: {}.".format(cluster_name))
    return cluster

def adjust_stack(pc, fp, sp):
    # pop sp, fp, pc from global stack
    gdb.execute('set $pc = {}'.format(pc))
    gdb.execute('set $rbp = {}'.format(fp))
    gdb.execute('set $sp = {}'.format(sp))

############################ COMMAND IMPLEMENTATION #########################

class Clusters(gdb.Command):
    """Print list of clusters"""
    usage_msg = """
    info clusters                   : print list of clusters"""

    def __init__(self):
        super(Clusters, self).__init__('info clusters', gdb.COMMAND_USER)

    def print_cluster(self, cluster_name, cluster_address):
        print('{:>20}  {:>20}'.format(cluster_name, cluster_address))

    #entry point from gdb
    def invoke(self, arg, from_tty):
        """
        Iterate through a circular linked list of clusters and print out its
        name along with address associated to each cluster
        @arg: str
        @from_tty: bool
        """
        uCPPTypes = None
        try:
            uCPPTypes = get_uCPP_types()
        except gdb.error:
            print(not_supported_error_msg)
            return

        argv = parse_argv_list(arg)
        if len(argv) != 0:
            print_usage(self.usage_msg)
            return

        cluster_root = get_cluster_root()
        if cluster_root.address == 0x0:
            return
        curr = cluster_root
        self.print_cluster('Name', 'Address')

        while True:
            self.print_cluster(curr['cluster_']['name'].string(),
                    str(curr['cluster_'].reference_value())[1:])
            curr = curr['next'].cast(uCPPTypes.uClusterDL_ptr_type)
            if curr == cluster_root:
                break

class ClusterProcessors(gdb.Command):
    """Print virtual processors in a given cluster (default userCluster)"""
    usage_msg = """
    info vprocessors                : print virtual processors in userCluster
    info vprocessors <clusterName>  : print virtual processors in cluster <clusterName>"""

    def __init__(self):
        super(ClusterProcessors, self).__init__('info vprocessors', gdb.COMMAND_USER)

    def print_processor(self, processor_address, pid, preemption, spin):
        print('{:>18}{:>20}{:>20}{:>20}'.format(processor_address, pid, preemption, spin))

    def invoke(self, arg, from_tty):
        """
        Iterate through a circular linked list of tasks and print out all
        info about each processor in that cluster
        @arg: str
        @from_tty: bool
        """
        uCPPTypes = None
        try:
            uCPPTypes = get_uCPP_types()
        except gdb.error:
            print(not_supported_error_msg)
            return

        argv = parse_argv_list(arg)
        if len(argv) > 1:
            print_usage(self.usage_msg)
            return

        if len(argv) == 0:
            cluster_address = lookup_cluster_by_name("userCluster")
        else:
            cluster_address = lookup_cluster_by_name(argv[0])

        if cluster_address == 0x0 or cluster_address == None:
            return

        processor_root = cluster_address.cast(uCPPTypes.uCluster_ptr_type)['processorsOnCluster']['root']
        if processor_root.address == 0x0:
            print('There are no processors for cluster at address: {}'.format(cluster_address))
            return

        uProcessorDL_ptr_type = gdb.lookup_type('uProcessorDL').pointer()
        self.print_processor('Address', 'PID', 'Preemption', 'Spin')
        curr = processor_root

        while True:
            processor = curr['processor_']
            self.print_processor(get_addr(processor.address),
                        str(processor['pid']), str(processor['preemption']),
                        str(processor['spin']))

            curr = curr['next'].cast(uProcessorDL_ptr_type)
            if curr == processor_root:
                break

class Task(gdb.Command):
    """Print information and switch stacks for tasks"""
    usage_msg = """
    task                            : print tasks (ids) in userCluster, application tasks only
    task <clusterName>              : print tasks (ids) in clusterName, application tasks only
    task all                        : print all clusters, all tasks
    task <id>                       : switch debugging stack to task id on userCluster
    task 0x<address>	            : switch debugging stack to task 0x<address> on any cluster
    task <id> <clusterName>         : switch debugging stack to task id on cluster clusterName
    """
    task_str_format = '{:>4}{:>20}{:>18}{:>25}'
    cluster_str_format = '{:>20}{:>18}'

    def __init__(self):
        # The first parameter of the line below is the name of the command. You
        # can call it 'uC++ task'
        super(Task, self).__init__('task', gdb.COMMAND_USER)

    ############################ AUXILIARY FUNCTIONS #########################

    def print_formatted_task(self, str_format, task_id, task_name, task_address, task_state):
        print(str_format.format(task_id, task_name, task_address, task_state))

    def print_formatted_tasks(self, task_id, break_addr, curr):
        if str(curr['task_'].reference_value())[1:] == break_addr:
            self.print_formatted_task(self.task_str_format, '* ' + str(task_id), curr['task_']['name_'].string(),
                    str(curr['task_'].reference_value())[1:],
                    str(curr['task_']['state_']))
        else:
            self.print_formatted_task(self.task_str_format, str(task_id), curr['task_']['name_'].string(),
                    str(curr['task_'].reference_value())[1:],
                    str(curr['task_']['state_']))

    def print_formatted_cluster(self, str_format, cluster_name, cluster_addr):
        print(str_format.format(cluster_name, cluster_addr))

    def find_curr_breakpoint_addr(self):
        btstr = gdb.execute('bt', to_string = True).splitlines()
        if len(btstr) < 3:
            print('Cannot locate current breakpoint')
            return None
        #1  0x000055555557745f in uMain::main (this=0x7fffffffe3e0) at driver.cc:86 <------ this address
        #2  0x000055555557ffa6 in UPP::uMachContext::invokeTask (This=...) at /usr/local/u++-7.0.0/src/kernel/uMachContext.cc:130
        #3  0x0000000000000000 in ?? ()
        return btstr[-3].split('this=',1)[1].split(',')[0].split(')')[0]

    def print_tasks_by_cluster_all(self, cluster_address):
        """
        Display a list of all info about all available tasks on a particular cluster
        @cluster_address: gdb.Value
        """
        uCPPTypes = None
        try:
            uCPPTypes = get_uCPP_types()
        except gdb.error:
            print(not_supported_error_msg)
            return

        cluster_address = cluster_address.cast(uCPPTypes.uCluster_ptr_type)
        task_root = cluster_address['tasksOnCluster']['root']

        if task_root == 0x0 or task_root.address == 0x0:
            print('There are no tasks for cluster at address: {}'.format(cluster_address))
            return

        self.print_formatted_task(self.task_str_format, 'ID', 'Task Name', 'Address', 'State')
        curr = task_root
        task_id = 0
        systask_id = -1

        breakpoint_addr = self.find_curr_breakpoint_addr()
        if breakpoint_addr is None:
            return

        while True:
            global SysTask_Name
            if (curr['task_']['name_'].string() in SysTask_Name):
                self.print_formatted_tasks(systask_id, breakpoint_addr, curr)
                systask_id -= 1
            else:
                self.print_formatted_tasks(task_id, breakpoint_addr, curr)
                task_id += 1

            curr = curr['next'].cast(uCPPTypes.uBaseTaskDL_ptr_type)
            if curr == task_root:
                break

    def print_tasks_by_cluster_address_all(self, cluster_address):
        """
        Display a list of all info about all available tasks on a particular cluster
        @cluster_address: str
        """
        # Iterate through a circular linked list of tasks and print out its
        # name along with address associated to each cluster

        # convert hex string to hex number
        try:
            hex_addr = int(cluster_address, 16)
        except:
            print_usage(self.usage_msg)
            return

        cluster_address = gdb.Value(hex_addr)
        if not self.print_tasks_by_cluster_all(cluster_address):
            return

    def print_tasks_by_cluster_address(self, cluster_address):
        """
        Display a list of limited info about all available tasks on a particular cluster
        @cluster_address: str
        """
        uCPPTypes = None
        try:
            uCPPTypes = get_uCPP_types()
        except gdb.error:
            print(not_supported_error_msg)
            return

        # Iterate through a circular linked list of tasks and print out its
        # name along with address associated to each cluster

        # convert hex string to hex number
        try:
            hex_addr = int(cluster_address, 16)
        except:
            print_usage(self.usage_msg)
            return

        cluster_address = gdb.Value(hex_addr).cast(uCPPTypes.uCluster_ptr_type)
        task_root = cluster_address['tasksOnCluster']['root']

        if task_root == 0x0 or task_root.address == 0x0:
            print('There are no tasks for cluster at address: {}'.format(cluster_address))
            return

        self.print_formatted_task(self.task_str_format, 'ID', 'Task Name', 'Address', 'State')
        curr = task_root
        task_id = 0
        breakpoint_addr = self.find_curr_breakpoint_addr()
        if breakpoint_addr is None:
            return

        while True:
            global SysTask_Name
            if (curr['task_']['name_'].string() not in SysTask_Name):
                self.print_formatted_tasks(task_id, breakpoint_addr, curr)

                curr = curr['next'].cast(uCPPTypes.uBaseTaskDL_ptr_type)
                task_id += 1
                if curr == task_root:
                    break
            else:
                curr = curr['next'].cast(uCPPTypes.uBaseTaskDL_ptr_type)
                if curr == task_root:
                    break

    ############################ COMMAND FUNCTIONS #########################

    def print_user_tasks(self):
        """Iterate only userCluster, print only tasks and main"""

        cluster_address = lookup_cluster_by_name("userCluster")
        if cluster_address == 0x0 or cluster_address == None:
            return

        self.print_tasks_by_cluster_address(str(cluster_address))

    def print_all_tasks(self):
        """Iterate through each cluster, iterate through all tasks and  print out info about all the tasks
        in those clusters"""
        uCPPTypes = None
        try:
            uCPPTypes = get_uCPP_types()
        except gdb.error:
            print(not_supported_error_msg)
            return

        cluster_root = get_cluster_root()
        if cluster_root.address == 0x0:
            return

        curr = cluster_root
        self.print_formatted_cluster(self.cluster_str_format, 'Cluster Name', 'Address')

        while True:
            addr = str(curr['cluster_'].reference_value())[1:]
            self.print_formatted_cluster(self.cluster_str_format, curr['cluster_']['name'].string(), addr)

            self.print_tasks_by_cluster_address_all(addr)
            curr = curr['next'].cast(uCPPTypes.uClusterDL_ptr_type)
            if curr == cluster_root:
                break

    def pushtask_by_address(self, task_address):
        """Change to a new task by switching to a different stack and manually
        adjusting sp, fp and pc
        @task_address: str
            2 supported format:
                in hex format
                    <hex_address>: literal hexadecimal address
                    Ex: 0xffffff
                in name of the pointer to the task
                    "task_name": pointer of the variable name of the cluster
                        Ex: T* s -> task_name = s
            Return: gdb.value of the cluster's address
        """
        uCPPTypes = None
        try:
            uCPPTypes = get_uCPP_types()
        except gdb.error:
            print(not_supported_error_msg)
            return

        # Task address has a format "task_address", which implies that it is the
        # name of the variable, and it needs to be evaluated
        if task_address.startswith('"') and task_address.endswith('"'):
            task = gdb.parse_and_eval(task_address.replace('"', ''))
        else:
        # Task address format does not include the quotation marks, which implies
        # that it is a hex address
            # convert hex string to hex number
            try:
                hex_addr = int(task_address, 16)
            except:
                print_usage(self.usage_msg)
                return
            task_address = gdb.Value(hex_addr)
            task = task_address.cast(uCPPTypes.uBaseTask_ptr_type)

        uContext_t_ptr_type = gdb.lookup_type('UPP::uMachContext::uContext_t').pointer()

        task_state = task['state_']
        if task_state == gdb.parse_and_eval('uBaseTask::Terminate'):
            print('Cannot switch to a terminated thread')
            return
        task_context = task['context_'].cast(uContext_t_ptr_type)

        # lookup for sp,fp and uSwitch
        uSwitch_offset = 48
        xsp = task_context['SP'] + uSwitch_offset
        xfp = task_context['FP']
        if not gdb.lookup_symbol('uSwitch'):
            print('uSwitch symbol is unavailable')
            return

        # convert string so we can strip out the address
        xpc = get_addr(gdb.parse_and_eval('uSwitch').address + 28)
        # must be at frame 0 to set pc register
        gdb.execute('select-frame 0')

        # push sp, fp, pc into a global stack
        global STACK
        sp = gdb.parse_and_eval('$sp')
        fp = gdb.parse_and_eval('$fp')
        pc = gdb.parse_and_eval('$pc')
        stack_info = StackInfo(sp = sp, fp = fp, pc = pc)
        STACK.append(stack_info)

        # update registers for new task
        gdb.execute('set $rsp={}'.format(xsp))
        gdb.execute('set $rbp={}'.format(xfp))
        gdb.execute('set $pc={}'.format(xpc))

    def find_matching_gdb_thread_id():
        """
        Parse the str from info thread to get the number
        """
        info_thread_str = gdb.execute('info thread', to_string=True).splitlines()
        for thread_str in info_thread_str:
            if thread_str.find('this={}'.format(task)) != -1:
                thread_id_pattern = r'^\*?\s+(\d+)\s+Thread'
                # retrive gdb thread id
                return re.match(thread_id_pattern, thread_str).group(1)

            # check if the task is running or not
            if task_state == gdb.parse_and_eval('uBaseTask::Running'):
                # find the equivalent thread from info thread
                gdb_thread_id = find_matching_gdb_thread_id()
                if gdb_thread_id is None:
                    print('Cannot find the thread id to switch to')
                    return
                # switch to that thread based using thread command
                gdb.execute('thread {}'.format(gdb_thread_id))

    def pushtask_by_id(self, task_id, cluster_name):
        """
        @cluster_name: str
        @task_id: str
        """
        uCPPTypes = None
        try:
            uCPPTypes = get_uCPP_types()
        except gdb.error:
            print(not_supported_error_msg)
            return

        try:
            task_id = int(task_id)
        except:
            print_usage(self.usage_msg)
            return

        # retrieve the address associated with the cluster name
        cluster_address = lookup_cluster_by_name(cluster_name)
        if cluster_address == 0x0 or cluster_address == None:
            return

        task_root = cluster_address.cast(uCPPTypes.uCluster_ptr_type)['tasksOnCluster']['root']
        if task_root == 0x0 or task_root.address == 0x0:
            print('There are no tasks on this cluster')
            return

        user_id = 0
        task_addr = None
        systask_id = -1 # system search id starts with negative

        # lookup for the task associated with the id
        global SysTask_Name
        if (task_id >= 0 and cluster_name == "systemCluster"):
            print('internal error: systemCluster does not have ID >= 0')
            return
        #id is a system task
        elif task_id < 0:
            curr = task_root
            rootflag = False
            while (curr['task_']['name_'].string() not in SysTask_Name):
                curr = curr['next'].cast(uCPPTypes.uBaseTaskDL_ptr_type)
                if curr == task_root:
                    rootflag = True
                    break
            if rootflag == False:
                if task_id == systask_id:
                    task_addr = str(curr['task_'].address)
                else:
                    while True:
                        curr = curr['next'].cast(uCPPTypes.uBaseTaskDL_ptr_type)

                        if (curr['task_']['name_'].string() in SysTask_Name):
                            systask_id -= 1
                            if curr == task_root:
                                break
                            if task_id == systask_id:
                                task_addr = str(curr['task_'].address)
                                break

                        if curr == task_root:
                            break
        #id is a user task
        else:
            curr = task_root
            rootflag = False
            while (curr['task_']['name_'].string() in SysTask_Name):
                curr = curr['next'].cast(uCPPTypes.uBaseTaskDL_ptr_type)
                if curr == task_root:
                    rootflag = True
                    break
            if rootflag == False:
                if task_id == user_id:
                    task_addr = str(curr['task_'].address)
                else:
                    while True:
                        curr = curr['next'].cast(uCPPTypes.uBaseTaskDL_ptr_type)

                        if (curr['task_']['name_'].string() not in SysTask_Name):
                            user_id += 1
                            if curr == task_root:
                                break
                            if task_id == user_id:
                                task_addr = str(curr['task_'].address)
                                break

                        if curr == task_root:
                            break

        if not task_addr:
            print("Cannot find task ID: {}. Only have {} tasks".format(task_id,user_id))
        else:
            self.pushtask_by_address(task_addr)

    def print_tasks_by_cluster_name(self, cluster_name):
        """
        Print out all the tasks available in the specified cluster
        @cluster_name: str
        """
        cluster_address = lookup_cluster_by_name(cluster_name)
        if cluster_address == 0x0 or cluster_address == None:
            return

        self.print_tasks_by_cluster_all(cluster_address)

    def invoke(self, arg, from_tty):
        """
        @arg: str
        @from_tty: bool
        """

        argv = parse_argv_list(arg)
        if len(argv) == 0:
            # print tasks
            self.print_user_tasks() # only tasks and main
        elif len(argv) == 1:
            # push task
            if argv[0].isdigit():
                self.pushtask_by_id(argv[0], "userCluster") # by id, userCluster
            elif argv[0].startswith('0x') or argv[0].startswith('0X'):
                self.pushtask_by_address(argv[0]) # by address, any cluster
            # print tasks
            elif argv[0] == 'all':
                self.print_all_tasks() # all tasks, all clusters
            else:
                self.print_tasks_by_cluster_name(argv[0]) # all tasks, specified cluster
        elif len(argv) == 2:
            # push task
            self.pushtask_by_id(argv[0], argv[1]) # by id, specified cluster
        else:
            print('parse error')
            print_usage(self.usage_msg)

class PrevTask(gdb.Command):
    """Switch back to previous task on the stack"""
    usage_msg = 'prevtask'

    def __init__(self):
        super(PrevTask, self).__init__('prevtask', gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        """
        @arg: str
        @from_tty: bool
        """
        global STACK
        if len(STACK) != 0:
            # must be at frame 0 to set pc register
            gdb.execute('select-frame 0')

            # pop stack
            stack_info = STACK.pop()
            pc = get_addr(stack_info.pc)
            sp = stack_info.sp
            fp = stack_info.fp

            # pop sp, fp, pc from global stack
            adjust_stack(pc, fp, sp)

            # must be at C++ frame to access C++ vars
            gdb.execute('frame 1')
        else:
            print('empty stack')

class ResetOriginFrame(gdb.Command):
    """Reset to the origin frame prior to continue execution again"""
    usage_msg = 'resetOriginFrame'
    def __init__(self):
        super(ResetOriginFrame, self).__init__('reset', gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        """
        @arg: str
        @from_tty: bool
        """
        global STACK
        if len(STACK) != 0:
            stack_info = STACK.pop(0)
            STACK.clear()
            pc = get_addr(stack_info.pc)
            sp = stack_info.sp
            fp = stack_info.fp

            # pop sp, fp, pc from global stack
            adjust_stack(pc, fp, sp)

            # must be at C++ frame to access C++ vars
            gdb.execute('frame 1')
        #else:
            #print('reset: empty stack') #probably does not have to print msg

Clusters()
ClusterProcessors()
ResetOriginFrame()
PrevTask()
Task()

# Local Variables: #
# mode: Python #
# End: #
