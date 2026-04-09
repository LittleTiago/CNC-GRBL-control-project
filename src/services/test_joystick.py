import sys
import tty
import termios

def get_key():
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch

def main():
    # Usamos stderr para que as mensagens apareçam na sua tela mesmo com o Pipe "|"
    sys.stderr.write("\n=== Openscience Joystick Ativo ===\n")
    sys.stderr.write("Controles: WASD | QE (Z) | H (Home) | R (Reset)\n")
    sys.stderr.write("Pressione ESC para sair.\n")
    sys.stderr.flush()
    
    step = 5.0 

    while True:
        key = get_key()
        
        # Sair com ESC ou Ctrl+C
        if key == '\x1b' or key == '\x03': 
            sys.stderr.write("\nEncerrando simulador...\n")
            break
        
        cmd = ""
        # Mapeamento de teclas (aceita maiúsculas ou minúsculas)
        k = key.lower()
        if k == 'w': cmd = f"REL 0 {step} 0"
        elif k == 's': cmd = f"REL 0 {-step} 0"
        elif k == 'a': cmd = f"REL {-step} 0 0"
        elif k == 'd': cmd = f"REL {step} 0 0"
        elif k == 'q': cmd = f"REL 0 0 {step}"
        elif k == 'e': cmd = f"REL 0 0 {-step}"
        elif k == 'h': cmd = "HOME"
        elif k == 'r': cmd = "RESET"
        
        if cmd:
            # 1. Envia o comando REAL para o C++ via stdout
            sys.stdout.write(cmd + "\n")
            sys.stdout.flush() # Importante: empurra o dado pelo "cano" na hora
            
            # 2. Avisa VOCÊ que a tecla funcionou via stderr
            sys.stderr.write(f" -> Tecla '{key}' detectada! Enviando: {cmd}\n")
            sys.stderr.flush()

if __name__ == "__main__":
    main()