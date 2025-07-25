using System;
using System.Windows.Input;

public class RelayCommand : ICommand {
    private readonly Action<object> _execute;
    private readonly Predicate<object> _canExecute;

    public RelayCommand(Action<object> execute, Predicate<object> ?canExecute = null) {
        _execute = execute ?? throw new ArgumentNullException(nameof(execute));
        _canExecute = canExecute!;
    }

    public bool CanExecute(object ?parameter) {
        if( parameter is not null ) {
            return _canExecute?.Invoke(parameter) ?? true;
        }
        else if( parameter is null ) {
            return false;
        }
        else {
            return false;
        }
    }

    public void Execute(object ?parameter) {
        _execute(parameter is not null);
    }

    public event EventHandler ?CanExecuteChanged {
        add => CommandManager.RequerySuggested += value;
        remove => CommandManager.RequerySuggested -= value;
    }
}
