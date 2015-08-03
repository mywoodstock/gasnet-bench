import sys, getopt

def extract_list(words):
  all = (' '.join(words)).split('=')[-1]
  all = [x.strip(',') for x in all.strip('[]').split()]
  all = map(int, all)
  return all

def is_int(s):
  try:
    int(s)
    return True
  except ValueError:
    return False

def process_tracefile(fname):
  data = []
  ff = open(fname)
  get_on = False
  while True:
    line = ff.readline()
    if not line: break
    words = line.strip().split()
    if words[0].endswith('>'): continue    ## ignore the info lines
    if not is_int(words[0]): continue
    proc = int(words[0])
    timestamp = float(words[1][0:-2])
    eventtype = words[2][1:-1]
    if eventtype == 'G':  ## get event
      get_on = True
      get_type = words[3][:-1]
      size = int(words[-1])
    elif eventtype == 'D':
      ## this assumes that (D) follows each (G)
      if get_on:
        type = words[3][:-1]
        if type != get_type:
          print 'error: found mismatched D event for G'
          break
        if get_type == 'GETS_NBI_BULK':
          newrec = {}
          newrec['node'] = int(words[7].split('=')[1])
          newrec['stridelevels'] = int(words[8].split('=')[1])
          newrec['count'] = extract_list(words[9:])
          dline = ff.readline()  ## ignore
          dline = ff.readline()
          dwords = dline.split()
          if dwords[0][:-1] != 'dst':
            print 'error: expected dst but instead found ' + dwords[0][:-1]
            break
          newrec['dststrides'] = extract_list(dwords[3:])
          dline = ff.readline() ## ignore
          dline = ff.readline() ## ignore
          dline = ff.readline()
          dwords = dline.split()
          if dwords[0][:-1] != 'src':
            print 'error: expected src but instead found ' + dwords[0][:-1]
            break
          newrec['srcstrides'] = extract_list(dwords[3:])
          dline = ff.readline() ## ignore
          dline = ff.readline() ## ignore
          data.append(newrec)
        else:
          print 'ignoring non GETS_NBI_BULK get operation ' + get_type 
        get_on = False
        get_type = ''
    elif eventtype == 'B':  ## barrier event
      continue
    elif eventtype == 'P':  ## put event
      continue
    elif eventtype == 'S':  ## sync event
      continue
    elif eventtype == 'W':
      continue
    elif eventtype == 'X':
      continue
    elif eventtype == 'N':
      continue
    else: continue      ## ignore any other event type
  ff.close()
  return data


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


nodeid = parse_arguments(sys.argv[1:])
fname = 'gasnet_trace_node' + str(nodeid)
print fname
data = process_tracefile(fname)

fname = 'gasnet_gets_operations' + str(nodeid) + '.ops'
write_data_detail(fname, data)
fname = 'gasnet_gets_operations' + str(nodeid) + '.raw'
write_data_raw(fname, data)
