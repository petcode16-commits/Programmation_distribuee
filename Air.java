import java.net.*;
import java.io.IOException;
import java.util.Random;

public class Air {
    private String nomPiece; // Utilisé pour les logs
    private double tempCourante;
    private double puissanceChauffage = 0;

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
                    MessageTemperature msg = MessageTemperature.fromBytes(dp.getData(), dp.getLength());
                    
                    // Filtrage par type (CHAUFFER = 1) et par nom de pièce
                    if (msg.getType() == MessageTemperature.CHAUFFER && msg.getPiece().equals(maPiece)) {
                        setPuissance(msg.getValeur());
                    }
                }
            } catch (IOException e) {
                // Arrêt normal du thread
            }
        }
    }

    private synchronized void setPuissance(int pwr) { 
        this.puissanceChauffage = pwr; 
    }

    public Air(String ipGroup, int port, String nom) throws Exception {
        this.nomPiece = nom;
        this.tempCourante = 18.0 + new Random().nextDouble() * 2;

        InetAddress group = InetAddress.getByName(ipGroup);
        InetSocketAddress groupAddress = new InetSocketAddress(group, port);
        
        // Sélection de l'interface réseau locale
        NetworkInterface netIf = NetworkInterface.getByInetAddress(InetAddress.getByName("127.0.0.1"));

        MulticastSocket s = new MulticastSocket(port);
        s.joinGroup(groupAddress, netIf);
        // setLoopbackMode supprimé (déprécié et inutile ici)

        new AttentePaquet(s, nom).start();

        System.out.println(">>> Simulation lancée pour la pièce : " + this.nomPiece);

        while (true) {
            // Physique simplifiée
            tempCourante += (puissanceChauffage * 0.4) - ((tempCourante - 15) * 0.05);
            
            // Envoi de la mesure (Type MESURE = 0)
            MessageTemperature m = new MessageTemperature((int)Math.round(tempCourante), MessageTemperature.MESURE, this.nomPiece);
            byte[] data = m.toBytes();
            
            s.send(new DatagramPacket(data, data.length, group, port));
            
            // Utilisation de nomPiece pour le log (règle le warning "not used")
            System.out.printf("\r[%s] Temp: %.1f°C | Puissance: %.0f ", this.nomPiece, tempCourante, puissanceChauffage);
            
            Thread.sleep(1000);
        }
    }

    public static void main(String[] args) {
        if (args.length < 3) {
            System.out.println("Usage: java Air <IP_Mcast> <Port> <Nom_Piece>");
            return;
        }
        try {
            new Air(args[0], Integer.parseInt(args[1]), args[2]);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}