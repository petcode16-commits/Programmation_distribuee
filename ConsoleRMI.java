import java.rmi.Naming;
import java.util.Scanner;

public class ConsoleRMI {
    public static void main(String[] args) {
        // Utilisation du try-with-resources pour fermer automatiquement le Scanner
        try (Scanner sc = new Scanner(System.in)) {
            // Connexion au registre RMI (on utilise 127.0.0.1 pour la robustesse)
            HomeInterface home = (HomeInterface) Naming.lookup("rmi://127.0.0.1/HomeAutomation");

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
                
                // 1. Lecture de l'état actuel (Monitoring) via RMI
                try {
                    int t = home.getTemperature(room);
                    String s = home.getHeatingStatus(room);
                    
                    System.out.println("\n--- État actuel de [" + room + "] ---");
                    System.out.println("Température : " + (t == -1 ? "Inconnue (En attente de données...)" : t + "°C"));
                    System.out.println("Chauffage   : " + s);
                    System.out.println("------------------------------------");
                } catch (Exception e) {
                    System.out.println("⚠️ La pièce [" + room + "] n'est pas encore enregistrée sur le serveur.");
                    continue;
                }

                // 2. Menu d'actions
                System.out.println("Options : 1.Manuel | 2.Auto | 3.Rafraîchir | 4.Retour");
                System.out.print("Votre choix : ");
                
                if (!sc.hasNextInt()) {
                    System.out.println("❌ Erreur : Veuillez entrer un chiffre entre 1 et 4.");
                    sc.next(); // Consomme l'entrée invalide
                    continue;
                }
                
                int choix = sc.nextInt();
                if (choix == 4) continue; // Retourne au choix de la pièce

                try {
                    if (choix == 1) {
                        int pwr = -1;
                        // BOUCLE DE VALIDATION PUISSANCE
                        while (pwr < 0 || pwr > 5) {
                            System.out.print("Saisir Puissance (Intervalle [0-5]) : ");
                            if (sc.hasNextInt()) {
                                pwr = sc.nextInt();
                                if (pwr < 0 || pwr > 5) {
                                    System.out.println("❌ Hors limites ! La puissance doit être comprise entre 0 et 5.");
                                }
                            } else {
                                System.out.println("❌ Erreur : Veuillez entrer un nombre entier.");
                                sc.next();
                            }
                        }
                        home.setPower(room, pwr);
                        System.out.println("✅ Ordre de puissance (" + pwr + ") envoyé avec succès.");

                    } else if (choix == 2) {
                        int temp = -1;
                        // BOUCLE DE VALIDATION TEMPÉRATURE
                        while (temp < 15 || temp > 30) {
                            System.out.print("Saisir Température cible (Intervalle [15-30] °C) : ");
                            if (sc.hasNextInt()) {
                                temp = sc.nextInt();
                                if (temp < 15 || temp > 30) {
                                    System.out.println("❌ Hors limites ! La température cible doit être entre 15 et 30°C.");
                                }
                            } else {
                                System.out.println("❌ Erreur : Veuillez entrer un nombre entier.");
                                sc.next();
                            }
                        }
                        home.setAuto(room, temp);
                        System.out.println("✅ Mode automatique (" + temp + "°C) activé avec succès.");

                    } else if (choix == 3) {
                        System.out.println("🔄 Mise à jour des données effectuée.");
                    } else {
                        System.out.println("⚠️ Choix non reconnu.");
                    }
                } catch (Exception e) {
                    System.err.println("❌ Erreur de communication distante : " + e.getMessage());
                }
            }
        } catch (Exception e) {
            System.err.println("❌ Erreur fatale Client RMI : " + e.getMessage());
            System.err.println("Vérifiez que le serveur RMIServer est bien lancé.");
        }
    }
}