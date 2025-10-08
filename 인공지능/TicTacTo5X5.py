# 5x5 보드는 1차원 리스트로 구현한다.
game_board = [' '] * 25

# 비어있는 칸을 찾아서 인덱스를 리스트로 반환한다.
def empty_cells(board):
    cells = []
    for x, cell in enumerate(board):
        if cell == ' ':
            cells.append(x)
    return cells

# 비어있는 칸에는 놓을 수 있다.
def valid_move(x):
    return x in empty_cells(game_board)

# 위치 x에 놓는다.
def move(x, player):
    if valid_move(x):
        game_board[x] = player
        return True
    return False

# 현재 게임보드를 그린다.
def draw(board):
    for i, cell in enumerate(board):
        if i % 5 == 0:
            print('\n-------------------------')
        print('|', cell, '|', end='')
    print('\n-------------------------')

# 5개의 연속된 문자가 수직, 수평, 또는 대각선으로 나타나면 승리
def check_win(board, player):
    size = 5
    win_len = 5
    # 행 체크
    for row in range(size):
        if all(board[row*size + col] == player for col in range(size)):
            return True
    # 열 체크
    for col in range(size):
        if all(board[row*size + col] == player for row in range(size)):
            return True
    # 대각선 체크 (\ 방향)
    if all(board[i*size + i] == player for i in range(size)):
        return True
    # 대각선 체크 (/ 방향)
    if all(board[i*size + (size-1-i)] == player for i in range(size)):
        return True
    return False

def evaluate(board):
    if check_win(board, 'X'):
        score = 1
    elif check_win(board, 'O'):
        score = -1
    else:
        score = 0
    return score

def game_over(board):
    return check_win(board, 'X') or check_win(board, 'O')

# 미니맥스 알고리즘
def minimax(board, depth, maxPlayer):
    pos = -1
    if depth == 0 or len(empty_cells(board)) == 0 or game_over(board):
        return -1, evaluate(board)
    if maxPlayer:
        value = -10000
        for p in empty_cells(board):
            board[p] = 'X'
            _, score = minimax(board, depth - 1, False)
            board[p] = ' '
            if score > value:
                value = score
                pos = p
    else:
        value = +10000
        for p in empty_cells(board):
            board[p] = 'O'
            _, score = minimax(board, depth - 1, True)
            board[p] = ' '
            if score < value:
                value = score
                pos = p
    return pos, value

# 메인 프로그램
player = 'X'
while True:
    draw(game_board)
    if len(empty_cells(game_board)) == 0 or game_over(game_board):
        break
    # 깊이(depth)를 5 이하로 낮추지 않으면 매우 느려질 수 있음
    i, v = minimax(game_board, 5, player == 'X')
    move(i, player)
    player = 'O' if player == 'X' else 'X'

draw(game_board)
if check_win(game_board, 'X'):
    print('X 승리!')
elif check_win(game_board, 'O'):
    print('O 승리!')
else:
    print('비겼습니다!')