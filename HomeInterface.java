import java.rmi.Remote;
import java.rmi.RemoteException;

public interface HomeInterface extends Remote {
    // Méthodes pour modifier l'état (Ecriture)
    void setPower(String room, int power) throws RemoteException;
    void setAuto(String room, int targetTemp) throws RemoteException;

    // Méthodes pour consulter l'état (Lecture)
    int getTemperature(String room) throws RemoteException;
    String getHeatingStatus(String room) throws RemoteException;
}