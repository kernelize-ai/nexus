import numpy
import torch

import nexus

buf0 = torch.ones(1024)
buf1 = torch.ones(1024)
res1 = torch.zeros(1024)
res2 = torch.zeros(1024)

for rt in nexus.get_runtimes():
  if (rt.get_devices().size()):
    dev = rt.get_devices()[0]
    if (dev.get_property_str(nexus.property.Type) == "gpu"):
      break

nb0 = dev.create_buffer(buf0)
nb1 = dev.create_buffer(buf1)
nb2 = dev.create_buffer(res1)
nb3 = dev.create_buffer(res2)

lib = dev.load_library("kernel.so")
kern = lib.get_kernel('add_vectors')

evf = dev.create_event()

stream0 = dev.create_stream()
stream1 = dev.create_stream()

# schedule 0
sched0 = dev.create_schedule()

cmd = sched0.create_command(kern)
cmd.set_arg(0, nb0)
cmd.set_arg(1, nb1)
cmd.set_arg(2, nb2)
cmd.finalize(32, 1024)

ev0 = sched0.create_signal()

# schedule 1
sched1 = dev.create_schedule()

sched1.create_wait(ev0.get_event())

cmd1 = sched1.create_command(kern)
cmd1.set_arg(0, nb0)
cmd1.set_arg(1, nb2)
cmd1.set_arg(2, nb3)
cmd1.finalize(32, 1024)

sched1.create_signal(evf)

# Run it
sched1.run(stream1, False)
sched0.run(stream0, False)

evf.wait()

nb3.copy(res2)

print(res2)
