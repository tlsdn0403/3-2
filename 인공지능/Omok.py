BOARD_SIZE = 9  # 빠른 테스트를 위한 9x9 오목판

game_board = [[' ' for _ in range(BOARD_SIZE)] for _ in range(BOARD_SIZE)]

def empty_cells(board):
    # 중앙에서 가까운 순으로 정렬하여 AI가 중앙에 두도록 함
    center = BOARD_SIZE // 2
    cells_with_dist = []
    for i in range(BOARD_SIZE):
        for j in range(BOARD_SIZE):
            if board[i][j] == ' ':
                dist = abs(center - i) + abs(center - j)
                cells_with_dist.append(((i, j), dist))
    cells_with_dist.sort(key=lambda x: x[1])
    return [pos for pos, _ in cells_with_dist]

def check_win(board, player):
    for i in range(BOARD_SIZE):
        for j in range(BOARD_SIZE):
            if board[i][j] != player:
                continue
            # 가로
            if j <= BOARD_SIZE - 5 and all(board[i][j+k] == player for k in range(5)):
                return True
            # 세로
            if i <= BOARD_SIZE - 5 and all(board[i+k][j] == player for k in range(5)):
                return True
            # 대각선 \
            if i <= BOARD_SIZE - 5 and j <= BOARD_SIZE - 5 and all(board[i+k][j+k] == player for k in range(5)):
                return True
            # 대각선 /
            if i <= BOARD_SIZE - 5 and j >= 4 and all(board[i+k][j-k] == player for k in range(5)):
                return True
    return False

def draw(board):
    # 열 번호 출력
    print("    " + "  ".join([f"{j:2}" for j in range(BOARD_SIZE)]))
    print("   +" + "---+" * BOARD_SIZE)
    for i, row in enumerate(board):
        print(f"{i:2} | " + " | ".join(row) + " |")
        print("   +" + "---+" * BOARD_SIZE)

def evaluate(board):
    # 중앙 보너스 포함, 더 정교한 평가
    def score_line(line, player):
        score = 0
        line_str = ''.join(line)
        # 5목 완성
        if player*5 in line_str:
            score += 100000
        # 열린 4 (양쪽이 비어있음)
        if f' {player*4} ' in line_str:
            score += 10000
        # 막힌 4 (한쪽만 비어있음)
        if f'{player*4} ' in line_str or f' {player*4}' in line_str:
            score += 2000
        # 열린 3
        if f' {player*3} ' in line_str:
            score += 300
        # 막힌 3
        if f'{player*3} ' in line_str or f' {player*3}' in line_str:
            score += 100
        # 열린 2
        if f' {player*2} ' in line_str:
            score += 30
        # 막힌 2
        if f'{player*2} ' in line_str or f' {player*2}' in line_str:
            score += 10
        return score

    total_score = 0
    # 모든 행, 열, 대각선 라인 평가
    for i in range(BOARD_SIZE):
        row = [game_board[i][j] for j in range(BOARD_SIZE)]
        col = [game_board[j][i] for j in range(BOARD_SIZE)]
        total_score += score_line(row, 'X')
        total_score -= score_line(row, 'O')
        total_score += score_line(col, 'X')
        total_score -= score_line(col, 'O')

    # 대각선 \
    for k in range(-BOARD_SIZE+1, BOARD_SIZE):
        diag1 = [game_board[i][i-k] for i in range(max(0, k), min(BOARD_SIZE, BOARD_SIZE+k))]
        if len(diag1) >= 5:
            total_score += score_line(diag1, 'X')
            total_score -= score_line(diag1, 'O')
    # 대각선 /
    for k in range(2*BOARD_SIZE-1):
        diag2 = [game_board[i][k-i] for i in range(max(0, k-BOARD_SIZE+1), min(BOARD_SIZE, k+1)) if 0 <= k-i < BOARD_SIZE]
        if len(diag2) >= 5:
            total_score += score_line(diag2, 'X')
            total_score -= score_line(diag2, 'O')
    # 중앙에 가까울수록 추가 보너스
    center = BOARD_SIZE // 2
    for i in range(BOARD_SIZE):
        for j in range(BOARD_SIZE):
            if game_board[i][j] == 'X':
                total_score += 5 - (abs(center - i) + abs(center - j))
            elif game_board[i][j] == 'O':
                total_score -= 5 - (abs(center - i) + abs(center - j))
    return total_score

def game_over(board):
    return check_win(board, 'X') or check_win(board, 'O')

def minimax(board, depth, maxPlayer, alpha=float('-inf'), beta=float('inf')):
    if depth == 0 or game_over(board):
        return None, evaluate(board)
    if maxPlayer:
        maxEval = float('-inf')
        best_move = None
        for (i, j) in empty_cells(board):
            board[i][j] = 'X'
            _, eval = minimax(board, depth-1, False, alpha, beta)
            board[i][j] = ' '
            if eval > maxEval:
                maxEval = eval
                best_move = (i, j)
            alpha = max(alpha, eval)
            if beta <= alpha:
                break  # 가지치기 (알파베타 프루닝)
        return best_move, maxEval
    else:
        minEval = float('inf')
        best_move = None
        for (i, j) in empty_cells(board):
            board[i][j] = 'O'
            _, eval = minimax(board, depth-1, True, alpha, beta)
            board[i][j] = ' '
            if eval < minEval:
                minEval = eval
                best_move = (i, j)
            beta = min(beta, eval)
            if beta <= alpha:
                break  # 가지치기 (알파베타 프루닝)
        return best_move, minEval

def main():
    draw(game_board)
    while True:
        # 인간(흑: O) 차례
        while True:
            try:
                user_input = input("당신의 차례입니다(O). 행 열(공백구분) 입력: ")
                i, j = map(int, user_input.strip().split())
                if 0 <= i < BOARD_SIZE and 0 <= j < BOARD_SIZE and game_board[i][j] == ' ':
                    game_board[i][j] = 'O'
                    break
                else:
                    print("빈 칸을 선택하세요.")
            except (ValueError, IndexError):
                print(f"올바른 좌표를 입력하세요 (예: 4 4, 0~{BOARD_SIZE-1} 범위).")
        draw(game_board)
        if check_win(game_board, 'O'):
            print("당신이 승리했습니다!")
            break
        if not empty_cells(game_board):
            print("무승부입니다!")
            break
        # 컴퓨터(백: X) 차례
        print("컴퓨터가 생각중입니다...")
        move, val = minimax(game_board, 3, True)  # 깊이 3로 AI 강화
        if move:
            game_board[move[0]][move[1]] = 'X'
        draw(game_board)
        if check_win(game_board, 'X'):
            print("컴퓨터가 승리했습니다!")
            break
        if not empty_cells(game_board):
            print("무승부입니다!")
            break

if __name__ == "__main__":
    main()