import java.net.*;
import java.io.IOException;
import java.util.Random;

/**
 * Air.java : Simule l'évolution thermique d'une pièce.
 * Version moderne (Java 14+) compatible avec le protocole binaire C.
 */
public class Air {
    private String nomPiece;
    private double tempCourante;
    private double puissanceChauffage = 0;

    /**
     * Thread d'écoute Multicast : Reçoit les messages CHAUFFER du module chauffage.c
     */
    protected class AttentePaquet extends Thread {
        private MulticastSocket socket;
        private String maPiece;

        public AttentePaquet(MulticastSocket s, String piece) {
            this.socket = s;
            this.maPiece = piece;
        }

        @Override
        public void run() {
            try {
                byte[] buf = new byte[1024];
                while (!isInterrupted()) {
                    DatagramPacket dp = new DatagramPacket(buf, buf.length);
                    socket.receive(dp);
                    
                    // Décodage du message binaire (Little-Endian)
                    MessageTemperature msg = MessageTemperature.fromBytes(dp.getData(), dp.getLength());
                    
                    // On ne traite que les messages de type CHAUFFER (1) pour cette pièce
                    if (msg.getType() == MessageTemperature.CHAUFFER && msg.getPiece().equals(maPiece)) {
                        setPuissance(msg.getValeur());
                    }
                }
            } catch (IOException e) {
                System.err.println("[" + maPiece + "] Arrêt de l'écouteur Multicast.");
            }
        }
    }

    // Mise à jour sécurisée de la puissance pour le thread principal
    private synchronized void setPuissance(int pwr) {
        this.puissanceChauffage = pwr;
    }

    public Air(String ipGroup, int port, String nom) throws Exception {
        this.nomPiece = nom;
        this.tempCourante = 17.0 + new Random().nextDouble() * 5; // Départ entre 17 et 22°C

        // --- CONFIGURATION MULTICAST MODERNE (Java 14+) ---
        InetAddress group = InetAddress.getByName(ipGroup);
        InetSocketAddress groupAddress = new InetSocketAddress(group, port);
        
        // Sélection de l'interface réseau (par défaut)
        NetworkInterface netIf = NetworkInterface.getByInetAddress(InetAddress.getLoopbackAddress()); 
        // Note: Sur certains systèmes, utiliser NetworkInterface.getNetworkInterfaces().nextElement() 
        // si le loopback ne suffit pas pour le Multicast.

        MulticastSocket s = new MulticastSocket(port);
        s.joinGroup(groupAddress, netIf);

        // Lancement du thread d'écoute
        new AttentePaquet(s, nom).start();

        System.out.println("========================================");
        System.out.println(" Simulation AIR lancée : " + nom);
        System.out.println(" Groupe : " + ipGroup + " | Port : " + port);
        System.out.println("========================================");

        // Boucle de simulation principale
        while (true) {
            calculerPhysique();

            // Envoi de la mesure (Type MESURE = 0)
            // On arrondit la température car le protocole attend un int
            MessageTemperature m = new MessageTemperature((int)Math.round(tempCourante), MessageTemperature.MESURE, nom);
            byte[] data = m.toBytes();
            
            DatagramPacket packet = new DatagramPacket(data, data.length, group, port);
            s.send(packet);

            // Pause de 1 seconde entre chaque cycle
            Thread.sleep(1000);
        }
    }

    /**
     * Calcule l'évolution de la température selon la puissance et l'extérieur.
     */
    private void calculerPhysique() {
        double tempExterieure = 12.0; // Température de base s'il n'y a pas de chauffage
        
        // Apport de chaleur (0.5°C par point de puissance par seconde)
        double apport = (puissanceChauffage * 0.5);
        
        // Déperdition thermique (5% de la différence avec l'extérieur)
        double perte = (tempCourante - tempExterieure) * 0.05;
        
        tempCourante += (apport - perte);

        // Affichage console propre (écrase la ligne précédente)
        System.out.printf("\r[%s] Statut: %s | Temp: %.2f°C | Puissance: %.0f  ", 
                          nomPiece, (puissanceChauffage > 0 ? "CHAUFFE" : "IDLE"), 
                          tempCourante, puissanceChauffage);
    }

    public static void main(String[] args) {
        if (args.length < 3) {
            System.out.println("Usage: java Air <IP_Multicast> <Port> <Nom_Piece>");
            return;
        }
        try {
            new Air(args[0], Integer.parseInt(args[1]), args[2]);
        } catch (Exception e) {
            System.err.println("Erreur fatale : " + e.getMessage());
            e.printStackTrace();
        }
    }
}