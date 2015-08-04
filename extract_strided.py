import sys, getopt

def parse_arguments(argv):
  try:
    opts, args = getopt.getopt(argv, 'n:')
    if len(opts) != 1:
      raise getopt.GetoptError('Specify node ID.')
  except getopt.GetoptError:
    print 'extract_trace.py -n <nodeid>'
    sys.exit(2)
  for opt, arg in opts:
    if opt in ('-n'): nnodes = int(arg)
    else:
      print 'wrong argument'
      sys.exit(2)
  return nnodes


def process_data(fname):
  strided = []
  nonstrided = []
  ff = open(fname)
  while True:
    line = ff.readline()
    if not line: break
    if line.startswith('#'): continue ## ignore the comment lines
    ## assume the following format: 5 lines
    ## node
    ## stridelevels
    ## count
    ## dststrides
    ## srcstrides
    node = int(line.strip().split()[0])
    line = ff.readline()
    stridelevels = int(line.strip().split()[0])
    line = ff.readline()
    count = map(int, line.strip().split())
    line = ff.readline()
    dststrides = map(int, line.strip().split())
    line = ff.readline()
    srcstrides = map(int, line.strip().split())
    newrec = {}
    newrec['node'] = node
    newrec['stridelevels'] = stridelevels
    newrec['count'] = count
    newrec['dststrides'] = dststrides
    newrec['srcstrides'] = srcstrides
    ## classify
    if (stridelevels == 0) or (stridelevels == 1 and count[1] == 1):
      nonstrided.append(newrec)
    else:
      strided.append(newrec)
  ff.close()
  return strided, nonstrided


def write_data_detail(fname, data):
  ff = open(fname, 'w')
  for d in data:
    ff.write(str(d) + '\n')
  ff.close()


def write_data_raw(fname, data):
  ff = open(fname, 'w')
  header = '## node\n## stridelevels\n## count\n## dststrides\n## srcstrides\n'
  ff.write(header)
  for d in data:
    ff.write(str(d['node']) + '\n')
    ff.write(str(d['stridelevels']) + '\n')
    ff.write(' '.join(map(str, d['count'])) + '\n')
    ff.write(' '.join(map(str, d['dststrides'])) + '\n')
    ff.write(' '.join(map(str, d['srcstrides'])) + '\n')
  ff.close()


nodeid = parse_arguments(sys.argv[1:])
fname = 'gasnet_gets_operations' + str(nodeid) + '.raw'
print fname
strided, nonstrided = process_data(fname)

fname = 'gasnet_gets_operations_strided' + str(nodeid) + '.ops'
write_data_detail(fname, strided)
fname = 'gasnet_gets_operations_strided' + str(nodeid) + '.raw'
write_data_raw(fname, strided)

fname = 'gasnet_gets_operations_nonstrided' + str(nodeid) + '.ops'
write_data_detail(fname, nonstrided)
fname = 'gasnet_gets_operations_nonstrided' + str(nodeid) + '.raw'
write_data_raw(fname, nonstrided)
