#!/usr/bin/env python3
"""
Test client for CXXRTL server POC
Connects to the server and exercises the protocol
"""

import socket
import json
import sys
import time


def send_message(sock, msg):
    """Send a JSON message with null terminator"""
    msg_str = json.dumps(msg) + '\0'
    print(f"Sending: {msg_str.strip()}")
    sock.sendall(msg_str.encode('utf-8'))


def receive_message(sock):
    """Receive a JSON message (null-terminated)"""
    msg = b''
    while True:
        chunk = sock.recv(1)
        if not chunk:
            return None
        if chunk == b'\0':
            break
        msg += chunk

    msg_str = msg.decode('utf-8')
    print(f"Received: {msg_str}")
    return json.loads(msg_str)


def main():
    # Connect to server
    print("Connecting to CXXRTL server on localhost:12345...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        sock.connect(('localhost', 12345))
        print("Connected!")

        # Send greeting
        print("\n=== Sending greeting ===")
        send_message(sock, {
            "type": "greeting",
            "version": 0
        })

        # Receive greeting response
        resp = receive_message(sock)
        if resp:
            print(f"Server capabilities:")
            print(f"  Commands: {resp.get('commands', [])}")
            print(f"  Events: {resp.get('events', [])}")
            print(f"  Features: {resp.get('features', {})}")

        # List scopes
        print("\n=== Listing scopes ===")
        send_message(sock, {
            "type": "command",
            "command": "list_scopes"
        })

        resp = receive_message(sock)
        if resp:
            scopes = resp.get('scopes', [])
            print(f"Available scopes: {scopes}")

        # List items in top scope
        print("\n=== Listing items in 'top' ===")
        send_message(sock, {
            "type": "command",
            "command": "list_items",
            "scope": "top"
        })

        resp = receive_message(sock)
        if resp:
            items = resp.get('items', [])
            print(f"Items in 'top':")
            for item in items:
                print(f"  {item['name']}: width={item['width']}, type={item['type']}")

        # Get simulation status
        print("\n=== Getting simulation status ===")
        send_message(sock, {
            "type": "command",
            "command": "get_simulation_status"
        })

        resp = receive_message(sock)
        if resp:
            print(f"Simulation time: {resp.get('time', 0)}")
            print(f"Running: {resp.get('running', False)}")

        print("\n=== Test complete ===")

    except ConnectionRefusedError:
        print("ERROR: Connection refused. Make sure the simulation is running.")
        print("Run: cd test_regress/t/poc_cxxrtl && make run")
        return 1
    except Exception as e:
        print(f"ERROR: {e}")
        import traceback
        traceback.print_exc()
        return 1
    finally:
        sock.close()

    return 0


if __name__ == '__main__':
    sys.exit(main())
