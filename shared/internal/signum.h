namespace marslander {

template<typename T>
int signum(T value) { return (T{} < value) - (value < T{}); }

} // namespace marslander
