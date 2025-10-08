# 보드는 1차원 리스트로 구현한다.
game_board = [' ', ' ', ' ',
              ' ', ' ', ' ',
              ' ', ' ', ' ']

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
        if i % 3 == 0:
            print('\n----------------')
        print('|', cell, '|', end='')
    print('\n----------------')

# 보드의 상태를 평가한다.
def evaluate(board):
    if check_win(board, 'X'):
        return 1
    elif check_win(board, 'O'):
        return -1
    else:
        return 0

# 승리 조건을 체크한다.
def check_win(board, player):
    win_conf = [
        [board[0], board[1], board[2]],
        [board[3], board[4], board[5]],
        [board[6], board[7], board[8]],
        [board[0], board[3], board[6]],
        [board[1], board[4], board[7]],
        [board[2], board[5], board[8]],
        [board[0], board[4], board[8]],
        [board[2], board[4], board[6]],
    ]
    return [player, player, player] in win_conf

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
        i, v = minimax(game_board, 9, True)
        print("\n컴퓨터(X)의 선택:", i)
        move(i, 'X')
        draw(game_board)
        if len(empty_cells(game_board)) == 0 or game_over(game_board):
            break
        # 인간 턴 (O)
        while True:
            try:
                human_move = int(input("당신의 차례입니다(O). 0~8 중 빈 칸 번호를 입력하세요: "))
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