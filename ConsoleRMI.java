import java.rmi.Naming;
import java.util.Scanner;

public class ConsoleRMI {
    public static void main(String[] args) {
        // Utilisation du try-with-resources pour fermer automatiquement le Scanner
        try (Scanner sc = new Scanner(System.in)) {
            HomeInterface home = (HomeInterface) Naming.lookup("rmi://localhost/HomeAutomation");

            System.out.println("========================================");
            System.out.println("   BIENVENUE SUR LA CONSOLE RMI JAVA   ");
            System.out.println("========================================");
            
            while (true) {
                System.out.print("\nNom de la pièce (ou 'exit' pour quitter) : ");
                String room = sc.next();
                
                if (room.equalsIgnoreCase("exit")) {
                    System.out.println("Fermeture de la console...");
                    break;
                }
                
                // 1. Lecture de l'état actuel (Monitoring)
                int t = home.getTemperature(room);
                String s = home.getHeatingStatus(room);
                
                System.out.println("\n--- État actuel de [" + room + "] ---");
                System.out.println("Température : " + (t == -1 ? "Inconnue" : t + "°C"));
                System.out.println("Chauffage   : " + s);
                System.out.println("------------------------------------");

                // 2. Menu d'actions
                System.out.println("Options : 1.Manuel(0-5) | 2.Auto(°C) | 3.Rafraîchir | 4.Quitter");
                System.out.print("Votre choix : ");
                
                // Sécurité pour vérifier si l'entrée est bien un nombre
                if (!sc.hasNextInt()) {
                    System.out.println("❌ Erreur : Veuillez entrer un chiffre.");
                    sc.next(); // Consommer l'entrée invalide
                    continue;
                }
                
                int choix = sc.nextInt();
                if (choix == 4) break;

                try {
                    if (choix == 1) {
                        System.out.print("Puissance (0-5) : ");
                        int pwr = sc.nextInt();
                        home.setPower(room, pwr);
                        System.out.println("✅ Ordre de puissance envoyé.");
                    } else if (choix == 2) {
                        System.out.print("Température cible (15-35) : ");
                        int temp = sc.nextInt();
                        home.setAuto(room, temp);
                        System.out.println("✅ Mode automatique activé.");
                    } else if (choix == 3) {
                        System.out.println("🔄 Mise à jour des données...");
                    } else {
                        System.out.println("⚠️ Choix non reconnu.");
                    }
                } catch (Exception e) {
                    // Capture les RemoteException (ex: puissance hors limite ou pièce non connectée)
                    System.err.println("❌ Erreur distante : " + e.getMessage());
                }
            }
        } catch (Exception e) {
            System.err.println("❌ Erreur fatale Client RMI : " + e.getMessage());
        }
        // Ici, le Scanner 'sc' est fermé automatiquement grâce au try-with-resources
    }
}