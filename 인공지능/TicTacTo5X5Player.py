# 보드는 1차원 리스트로 구현한다.
game_board = [' '] * 25

# 비어있는 칸을 찾아서 인덱스를 리스트로 반환한다.
def empty_cells(board):
    return [x for x, cell in enumerate(board) if cell == ' ']

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

# 보드의 상태를 평가한다.
def evaluate(board):
    if check_win(board, 'X'):
        return 1
    elif check_win(board, 'O'):
        return -1
    else:
        return 0

# 승리 조건을 체크한다.
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
            x, score = minimax(board, depth - 1, False)
            board[p] = ' '
            if score > value:
                value = score
                pos = p
    else:
        value = +10000
        for p in empty_cells(board):
            board[p] = 'O'
            x, score = minimax(board, depth - 1, True)
            board[p] = ' '
            if score < value:
                value = score
                pos = p
    return pos, value

# 메인 프로그램
def main():
    while True:
        draw(game_board)
        if len(empty_cells(game_board)) == 0 or game_over(game_board):
            break
        # 컴퓨터 턴 (X)
        # 깊이(depth)를 5 이하로 낮추지 않으면 매우 느려질 수 있음
        i, v = minimax(game_board, 3, True)
        print("\n컴퓨터(X)의 선택:", i)
        move(i, 'X')
        draw(game_board)
        if len(empty_cells(game_board)) == 0 or game_over(game_board):
            break
        # 인간 턴 (O)
        while True:
            try:
                human_move = int(input("당신의 차례입니다(O). 0~24 중 빈 칸 번호를 입력하세요: "))
                if move(human_move, 'O'):
                    break
                else:
                    print("잘못된 입력입니다. 다시 입력하세요.")
            except ValueError:
                print("숫자를 입력하세요.")
    # 결과 출력
    if check_win(game_board, 'X'):
        print('컴퓨터(X) 승리!')
    elif check_win(game_board, 'O'):
        print('당신(O) 승리!')
    else:
        print('비겼습니다!')

if __name__ == "__main__":
    main()