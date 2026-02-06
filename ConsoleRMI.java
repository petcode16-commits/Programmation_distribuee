import java.rmi.Naming;
import java.util.Scanner;

public class ConsoleRMI {
    public static void main(String[] args) {
        try {
            HomeInterface home = (HomeInterface) Naming.lookup("rmi://localhost/HomeAutomation");
            Scanner sc = new Scanner(System.in);

            while (true) {
                System.out.print("\nEntrez le nom de la pièce à consulter/piloter : ");
                String room = sc.next();
                
                // Affichage de l'état actuel (Lecture distante)
                int t = home.getTemperature(room);
                String s = home.getHeatingStatus(room);
                
                System.out.println("--- État actuel de [" + room + "] ---");
                System.out.println("Température : " + (t == -1 ? "Donnée indisponible" : t + "°C"));
                System.out.println("Chauffage   : " + s);
                System.out.println("------------------------------");

                System.out.println("Options: 1.Manuel(0-5) 2.Auto(°C) 3.Rafraîchir 4.Quitter");
                int choix = sc.nextInt();
                if (choix == 4) break;
                if (choix == 1) {
                    System.out.print("Puissance : ");
                    home.setPower(room, sc.nextInt());
                } else if (choix == 2) {
                    System.out.print("Cible °C : ");
                    home.setAuto(room, sc.nextInt());
                }
            }
        } catch (Exception e) { e.printStackTrace(); }
    }
}