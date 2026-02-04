import java.rmi.Naming;
import java.util.Scanner;

public class ConsoleRMI {
    public static void main(String[] args) {
        try {
            RemoteConsole service = (RemoteConsole) Naming.lookup("rmi://localhost/DomotiqueService");
            try (Scanner sp = new Scanner(System.in)) {
                while(true) {
                    System.out.println("\n1: Voir températures | 2: Chauffer | 3: Quitter");
                    int choix = sp.nextInt(); sp.nextLine();
                    if (choix == 1) {
                        System.out.println(service.getTemperatures());
                    } else if (choix == 2) {
                        System.out.print("Pièce : "); String p = sp.nextLine();
                        System.out.print("Valeur (0-5) : "); int v = sp.nextInt();
                        service.commanderChauffage(p, v);
                    } else break;
                }
            }
        } catch (Exception e) { e.printStackTrace(); }
    }
}