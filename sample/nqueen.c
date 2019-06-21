#define N 8

bool conflict(int *board, int row, int col) {
  for (int i = 0; i < row; i++) {
    if (board[i * N + col]) {
      return 1;
    }
    int j = row - i;
    if (col - j + 1 > 0 && board[i * N + col - j]) {
      return 1;
    }
    if (col + j < N && board[i * N + col + j]) {
      return 1;
    }
  }
  return 0;
}

int solve(int *board, int row) {
  if (row == N) {
    return 1;
  }
  int count = 0;
  for (int i = 0; i < N; i++) {
    if (!conflict(board, row, i)) {
      board[row * N + i] = 1;
      count += solve(board, row + 1);
      board[row * N + i] = 0;
    }
  }
  return count;
}

int main() {
  int board[N * N];
  for (int i = 0; i < N * N; i++) {
    board[i] = 0;
  }
  printf("count: %d\n", solve(board, 0));
}
