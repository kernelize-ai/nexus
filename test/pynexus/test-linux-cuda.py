import numpy

import sys
#import subprocess
#subprocess.check_call([sys.executable, "-m", "pip", "install", "torch"])

import torch
import nexus

buf0 = torch.ones(1024)
buf1 = torch.ones(1024)
res2 = torch.zeros(1024)


rt = nexus.get_runtime("cuda")

dev = rt.get_devices()[0]

#nb0 = dev.create_buffer(buf0)
nb0 = dev.create_buffer(buf0)

nb1 = dev.create_buffer(buf1)
nb2 = dev.create_buffer(res2)

lib = dev.load_library("build.local/kernel_libs/add_vectors_shmem.ptx")
kern = lib.get_kernel('add_vectors_shmem')

sched = dev.create_schedule()

cmd = sched.create_command(kern)
cmd.set_arg(0, nb0)
cmd.set_arg(0, nb0)
cmd.set_arg(1, nb1)
cmd.set_arg(2, nb2)
cmd.finalize([32,1,1], [32,1,1], 2048)

sched.run()

#res = nr2.get()

nb2.copy(res2)

print(res2)
