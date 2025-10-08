import queue

# 2021182014 박신우
class State:
    def __init__(self, queens, n, depth=0):
        self.queens = queens  # 각 행에 놓인 퀸의 열 위치 리스트 
        self.n = n
        self.depth = depth    # 현재까지 놓은 퀸의 수

  # 남은 행마다 퀸을 놓을 수 없는 값을 더해서 반환함
    def h(self):
        score = 0
        for next_row in range(self.depth, self.n): #range(a,b) a부터 b-1까지
            safe_cols = 0
            for col in range(self.n): #range(n0 = 0부터 n-1까지)
                conflict = False # true면 퀸 놓을 수 없음
                for r, c in enumerate(self.queens): #r은 index , c는 value
                    if c == col or abs(c - col) == abs(next_row - r): #같은열 , 같은 대각선 중 하나
                        conflict = True  #이면 충돌
                        break
                if not conflict:
                    safe_cols += 1
            # 놓을 수 없는 칸이 많을수록 score 높임
            score += (self.n - safe_cols)  #놓을 수 있는 곳 만큼 빼줌
        return score

    def g(self):
        return self.depth

    def f(self):
        return self.g() + self.h()

    def is_valid(self, col):
        row = self.depth
        for r, c in enumerate(self.queens):
            # 같은 열, 같은 대각선에 퀸이 있는지 체크
            if c == col or abs(c - col) == abs(row - r):
                return False
        return True

    def expand(self):
        result = []
        if self.depth < self.n:
            for col in range(self.n):
                if self.is_valid(col):
                    # 새 퀸을 추가한 새로운 상태 만듦
                    new_queens = self.queens + [col]
                    result.append(State(new_queens, self.n, self.depth + 1))
        return result

    def __eq__(self, other):
        return self.queens == other.queens

    def __lt__(self, other):
        return self.f() < other.f()

    def __str__(self):
        board = []
        for i in range(self.n):
            row = ['.'] * self.n
            if i < len(self.queens):
                row[self.queens[i]] = 'Q'
            board.append(' '.join(row))
        return f"f(n)={self.f()} h(n)={self.h()} g(n)={self.g()}\n" + '\n'.join(board) + '\n'

def nqueen_astar(n):
    open_queue = queue.PriorityQueue()
    open_queue.put(State([], n))
    closed_queue = []
    count = 0

    while not open_queue.empty():
        current = open_queue.get()
        count += 1
        print(f"Step {count}")
        print(current)
        if current.depth == n:  # 모든 퀸이 놓였으면 성공
            print("탐색 성공")
            return current
        for state in current.expand():
            if state not in closed_queue and state not in open_queue.queue:
                open_queue.put(state)
        closed_queue.append(current)
    print("탐색 실패")
    return None

if __name__ == "__main__":
    print("2021182014 박신우")
    N = int(input("N을 입력하세요: "))
    solution = nqueen_astar(N)
    if solution:
        print("최종 보드:")
        print(solution)
    else:
        print("해가 없습니다.")