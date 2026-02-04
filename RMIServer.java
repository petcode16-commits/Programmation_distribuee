import java.rmi.Naming;
import java.rmi.RemoteException;
import java.rmi.server.UnicastRemoteObject;
import java.net.*;

public class RMIServer extends UnicastRemoteObject implements RemoteConsole {
    private final int UDP_PORT_CENTRAL = 9090; // Le port où le programme C écoute

    protected RMIServer() throws RemoteException { super(); }

    @Override
    public String getTemperatures() throws RemoteException {
        try (DatagramSocket socket = new DatagramSocket()) {
            // 1. Envoyer la requête "LIST" au programme C via UDP
            byte[] sendBuf = "LIST".getBytes();
            InetAddress address = InetAddress.getByName("localhost");
            DatagramPacket packet = new DatagramPacket(sendBuf, sendBuf.length, address, UDP_PORT_CENTRAL);
            socket.send(packet);

            // 2. Attendre la réponse du programme C
            byte[] recvBuf = new byte[2048];
            DatagramPacket recvPacket = new DatagramPacket(recvBuf, recvBuf.length);
            socket.setSoTimeout(1000); // Temps mort de 1s
            socket.receive(recvPacket);
            return new String(recvPacket.getData(), 0, recvPacket.getLength());
        } catch (Exception e) {
            return "Erreur : Impossible de joindre le Central (C) en UDP sur le port " + UDP_PORT_CENTRAL;
        }
    }

    @Override
    public void commanderChauffage(String piece, int puissance) throws RemoteException {
        try (DatagramSocket socket = new DatagramSocket()) {
            // Envoyer la commande au format "CMD Salon 3" au programme C
            String msg = "CMD " + piece + " " + puissance;
            byte[] buf = msg.getBytes();
            DatagramPacket packet = new DatagramPacket(buf, buf.length, InetAddress.getByName("localhost"), UDP_PORT_CENTRAL);
            socket.send(packet);
        } catch (Exception e) { e.printStackTrace(); }
    }

    public static void main(String[] args) {
        try {
            java.rmi.registry.LocateRegistry.createRegistry(1099); // Lancer le registre RMI
            RMIServer server = new RMIServer();
            Naming.rebind("DomotiqueService", server);
            System.out.println("Pont RMI-UDP prêt sur le port 1099...");
        } catch (Exception e) { e.printStackTrace(); }
    }
}