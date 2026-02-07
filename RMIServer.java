import java.rmi.server.UnicastRemoteObject;
import java.rmi.registry.LocateRegistry;
import java.rmi.RemoteException;
import java.net.*;

// Imports pour la gestion des données
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class RMIServer extends UnicastRemoteObject implements HomeInterface {
    
    // Cache local pour stocker les états des pièces envoyés par le Central C
    private Map<String, Integer> temperatures = new ConcurrentHashMap<>();
    private Map<String, String> status = new ConcurrentHashMap<>();

    private static final String CENTRAL_HOST = "localhost";
    private static final int CENTRAL_UDP_PORT_IN = 9090;  // Port pour envoyer des ordres
    private static final int RMI_UDP_PORT_LISTEN = 9091; // Port pour recevoir les updates (STAT/TEMP)

    protected RMIServer() throws RemoteException {
        super();
        // Lancement du thread qui écoute les mises à jour du Central
        startStateListener();
    }

    /**
     * Thread d'écoute UDP : Reçoit les messages TEMP et STAT du serveur Central
     * pour maintenir le cache à jour.
     */
    private void startStateListener() {
        Thread t = new Thread(() -> {
            try (DatagramSocket socket = new DatagramSocket(RMI_UDP_PORT_LISTEN)) {
                byte[] buffer = new byte[512];
                System.out.println("[RMI Cache] Écoute des mises à jour sur le port " + RMI_UDP_PORT_LISTEN);
                
                while (true) {
                    DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
                    socket.receive(packet);
                    String msg = new String(packet.getData(), 0, packet.getLength()).trim();
                    
                    // Analyse des messages (Format: "UPDATE Salon 22" ou "STAT Salon AUTO_ON 3")
                    if (msg.startsWith("UPDATE")) {
                        String[] parts = msg.split(" ");
                        if (parts.length >= 3) {
                            temperatures.put(parts[1], Integer.parseInt(parts[2]));
                        }
                    } else if (msg.startsWith("STAT")) {
                        String[] parts = msg.split(" ");
                        if (parts.length >= 4) {
                            // On stocke un résumé : Statut (ex: AUTO_ON) et Puissance (ex: 3)
                            status.put(parts[1], parts[2] + " (Pwr: " + parts[3] + ")");
                        }
                    }
                }
            } catch (Exception e) {
                System.err.println("[RMI Cache Error] " + e.getMessage());
            }
        });
        t.setDaemon(true); // Le thread s'arrête si le serveur s'arrête
        t.start();
    }

    /**
     * Envoie un message brut en UDP Unicast au serveur Central C
     */
    private void sendToCentral(String message) {
        try (DatagramSocket socket = new DatagramSocket()) {
            byte[] buf = message.getBytes();
            InetAddress address = InetAddress.getByName(CENTRAL_HOST);
            DatagramPacket packet = new DatagramPacket(buf, buf.length, address, CENTRAL_UDP_PORT_IN);
            socket.send(packet);
            System.out.println("[RMI -> Central] Commande envoyée : " + message);
        } catch (Exception e) {
            System.err.println("[RMI Error] Échec envoi Central : " + e.getMessage());
        }
    }

    // --- Implémentation de HomeInterface ---

    @Override
    public synchronized void setPower(String room, int power) throws RemoteException {
        if (power < 0 || power > 5) {
            throw new RemoteException("Erreur : La puissance doit être entre 0 et 5.");
        }
        sendToCentral("SET " + room + " " + power);
    }

    @Override
    public synchronized void setAuto(String room, int targetTemp) throws RemoteException {
        if (targetTemp < 15 || targetTemp > 35) {
            throw new RemoteException("Erreur : Température cible invalide (15-35°C).");
        }
        sendToCentral("AUTO " + room + " " + targetTemp);
    }

    @Override
    public int getTemperature(String room) throws RemoteException {
        // Retourne la température ou -1 si la pièce n'est pas encore connue
        return temperatures.getOrDefault(room, -1);
    }

    @Override
    public String getHeatingStatus(String room) throws RemoteException {
        return status.getOrDefault(room, "Inconnu (En attente de données...)");
    }

    public static void main(String[] args) {
        try {
            // Création du registre RMI sur le port 1099
            LocateRegistry.createRegistry(1099);
            
            RMIServer server = new RMIServer();
            java.rmi.Naming.rebind("HomeAutomation", server);
            
            System.out.println("============================================");
            System.out.println("      SERVEUR RMI OPÉRATIONNEL    ");
            System.out.println("============================================");
            System.out.println("En attente de requêtes clients...");
        } catch (Exception e) {
            System.err.println("[RMI Fatal Error] " + e.getMessage());
            e.printStackTrace();
        }
    }
}