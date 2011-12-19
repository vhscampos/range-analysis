V = [0, 1, 2, 3, 4, 5, 5, 6, 7]
E = {(0, 1):10, (2, 1):1, (2, 3):1, (3, 4):3, (4, 5):-1, (5, 2):-2, (6, 5):-1,
  (6, 1):-4, (7, 6):1, (0, 7):8, (1, 5):2}

dist = range(len(V))
prev = range(len(V))

def update((u, v)):
  dist[v] = min(dist[v], dist[u] + E[(u, v)])

def shortest_path(s):
  for u in V:
    dist[u] = 1000000
    prev[u] = None
  dist[s] = 0
  for i in range(len(V)):
    for e in E.keys():
      update(e)
  for u in V:
    print dist[u]

shortest_path(0)
