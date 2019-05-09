int conflict(int *board, int row, int col) {
  for (int i = 0; i < row; i++) {
    if (board[i * 8 + col]) {
      return 1;
    }
    int j = row - i;
    if (col - j + 1 > 0 && board[i * 8 + col - j]) {
      return 1;
    }
    if (col + j < 8 && board[i * 8 + col + j]) {
      return 1;
    }
  }
  return 0;
}

int solve(int *board, int row) {
  if (row == 8) {
    return 1;
  }
  int count = 0;
  for (int i = 0; i < 8; i++) {
    if (!conflict(board, row, i)) {
      board[row * 8 + i] = 1;
      count += solve(board, row + 1);
      board[row * 8 + i] = 0;
    }
  }
  return count;
}

int main() {
  int board[64];
  for (int i = 0; i < 64; i++) {
    board[i] = 0;
  }
  printf("count: %d\n", solve(board, 0));
}
