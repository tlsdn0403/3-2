class State:
    def __init__(self, board, goal, depth=0, parent=None, move=None):
        self.board = board
        self.goal = goal
        self.depth = depth
        self.parent = parent
        self.move = move

    def get_new_board(self, i1, i2, depth, move):
        new_board = self.board[:]
        new_board[i1], new_board[i2] = new_board[i2], new_board[i1]
        return State(new_board, self.goal, depth, self, move)

    def expand(self, depth):
        result = []
        i = self.board.index(0)
        if i not in [0, 3, 6]:  # LEFT
            result.append(self.get_new_board(i, i-1, depth, "LEFT"))
        if i not in [0, 1, 2]:  # UP
            result.append(self.get_new_board(i, i-3, depth, "UP"))
        if i not in [2, 5, 8]:  # RIGHT
            result.append(self.get_new_board(i, i+1, depth, "RIGHT"))
        if i not in [6, 7, 8]:  # DOWN
            result.append(self.get_new_board(i, i+3, depth, "DOWN"))
        return result

   
    def __str__(self):
        return (
            str(self.board[:3]) + "\n"
            + str(self.board[3:6]) + "\n"
            + str(self.board[6:]) + "\n"
            + "------------------"
        )
    def __eq__(self, other):
        return self.board == other.board
    def __hash__(self):
        return hash(tuple(self.board))
    


def is_goal(state):
    return state.board == state.goal

def DFS(node, depth, visited):
    if is_goal(node):
        return node, True    # found, remaining
    if depth == 0:
        return None, True    # 남은 노드 있음
    visited.add(node)
    any_remaining = False
    for child in node.expand(node.depth + 1):
        if child not in visited:
            found, remaining = DFS(child, depth-1, visited)
            if found is not None:
                return found, True
            if remaining:
                any_remaining = True
    visited.remove(node)
    return None, any_remaining

def IDDFS(root, max_depth=30):
    for depth in range(max_depth):
        visited = set()
        found, remaining = DFS(root, depth, visited)
        if found is not None:
            return found
        elif not remaining:
            return None
    return None

def reconstruct_path(state):
    path = []
    while state.parent:
        path.append((state.move, state.board))
        state = state.parent
    path.reverse()
    return path

if __name__ == "__main__":
    # 초기상태
    puzzle = [2, 8, 3, 1, 6, 4, 7, 0, 5]
    # 목표상태
    goal = [1, 2, 3, 8, 0, 4, 7, 6, 5]
    start = State(puzzle, goal)
    print("풀이 시작")
    result = IDDFS(start, max_depth=20)
    if result:
        print("탐색 성공")
        path = reconstruct_path(result)
        print("최적 경로:")
        for move, board in path:
            print(f"Move: {move}\n {board[:3]}\n {board[3:6]}\n {board[6:]}\n")
        print(f"총 이동 횟수: {len(path)}")
    else:
        print("해결 경로를 찾지 못했습니다.")