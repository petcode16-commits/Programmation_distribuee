import java.net.*;
import java.io.IOException;
import java.util.Random;

/**
 * Air.java : Simule la température d'une pièce.
 * Diffuse la température en Multicast et écoute les ordres de chauffage.
 */
public class Air {
    private String nomPiece;
    private double tempCourante;
    private double puissanceChauffage = 0;

    /**
     * Thread qui écoute en permanence les messages Multicast (Type CHAUFFER)
     */
    protected class AttentePaquet extends Thread {
        protected int dernierCommande = -1;
        protected MulticastSocket socket;
        protected String maPiece;

        public AttentePaquet(MulticastSocket s, String piece) {
            this.socket = s;
            this.maPiece = piece;
        }

        public synchronized int getDernier() {
            int temp = dernierCommande;
            dernierCommande = -1;
            return temp;
        }

        public void run() {
            try {
                byte[] buf = new byte[1024];
                while (!isInterrupted()) {
                    DatagramPacket dp = new DatagramPacket(buf, buf.length);
                    socket.receive(dp);
                    
                    // On décode le message reçu via la méthode statique de MessageTemperature
                    MessageTemperature msg = MessageTemperature.fromBytes(dp.getData(), dp.getLength());
                    
                    // On ne traite que si c'est un ordre de CHAUFFAGE pour NOTRE pièce
                    if (msg.type == MessageTemperature.CHAUFFER && msg.piece.equals(maPiece)) {
                        synchronized(this) {
                            dernierCommande = msg.valeur;
                        }
                    }
                }
            } catch (IOException e) {
                System.err.println("Fin de l'écoute Multicast : " + e.getMessage());
            }
        }
    }

    public Air(String ipGroup, int port, String nom) throws Exception {
        this.nomPiece = nom;
        this.tempCourante = 18.0 + new Random().nextDouble() * 5; // Température initiale aléatoire
        
        // Initialisation Multicast (Version Corrigée et Moderne)
        InetAddress group = InetAddress.getByName(ipGroup);
        MulticastSocket s = new MulticastSocket(port);
        
        // Rejoindre le groupe (Indispensable pour recevoir des messages)
        s.joinGroup(new InetSocketAddress(group, port), NetworkInterface.getByInetAddress(InetAddress.getLocalHost()));

        // Lancement du thread d'écoute
        AttentePaquet ecouteur = new AttentePaquet(s, nom);
        ecouteur.start();

        System.out.println("Simulation démarrée pour la pièce : " + nom);

        // Boucle principale de simulation
        int cycleChauffage = 0;
        while (true) {
            // 1. Mise à jour de la température (logique simplifiée)
            calculerEvolution();

            // 2. Toutes les 3 secondes, on vérifie si un nouvel ordre de chauffage est arrivé
            if (cycleChauffage >= 3) {
                int nouvellePuissance = ecouteur.getDernier();
                if (nouvellePuissance != -1) {
                    this.puissanceChauffage = nouvellePuissance;
                    System.out.println("[" + nom + "] Chauffage réglé sur : " + puissanceChauffage);
                }
                cycleChauffage = 0;
            }

            // 3. Envoi de la température actuelle (Type MESURE)
            MessageTemperature m = new MessageTemperature((int)tempCourante, MessageTemperature.MESURE, nom);
            byte[] data = m.toBytes();
            s.send(new DatagramPacket(data, data.length, group, port));

            Thread.sleep(1000); // Une itération par seconde
            cycleChauffage++;
        }
    }

    private void calculerEvolution() {
        // Logique simplifiée de l'évolution de la température :
        // La température tend vers l'extérieur, mais augmente avec le chauffage.
        double tempExt = 15.0; // Simplification pour l'exemple
        double apportChauffage = (puissanceChauffage * 0.5);
        double deperdition = (tempCourante - tempExt) * 0.1;
        
        tempCourante += (apportChauffage - deperdition);
        
        // Limites physiques
        if (tempCourante < -10) tempCourante = -10;
        if (tempCourante > 40) tempCourante = 40;
        
        // Debug: affiche l'état de la pièce
        System.out.println("[" + nomPiece + "] Température: " + String.format("%.2f", tempCourante) + "°C");
    }

    public static void main(String[] args) {
        if (args.length < 3) {
            System.out.println("Usage: java Air <IP_Multicast> <Port> <Nom_Piece>");
            return;
        }
        try {
            new Air(args[0], Integer.parseInt(args[1]), args[2]);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}