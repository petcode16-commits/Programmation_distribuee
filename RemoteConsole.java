import java.rmi.Remote;
import java.rmi.RemoteException;

public interface RemoteConsole extends Remote {
    // Récupérer la liste des températures formatée en texte
    String getTemperatures() throws RemoteException;
    
    // Envoyer un ordre de chauffage (0 à 5) pour une pièce donnée
    void commanderChauffage(String piece, int puissance) throws RemoteException;
}