export class Contactinfo {
    private _email: string = '';
    private _phone: string = '';
    private _address: Address;
    private _previousAddresses: Array<Address> = [];

    constructor() {
        this._previousAddresses = [];
    }

    get email(): string {
        return this._email;
    }

    set email(value: string) {
        this._email = value;
    }

    get phone(): string {
        return this._phone;
    }

    set phone(value: string) {
        this._phone = value;
    }

    get address(): Address {
        return this._address;
    }

    set address(value: Address) {
        this._address = value;
    }

    get previousAddresses(): Array<Address> {
        return this._previousAddresses;
    }

    set previousAddresses(value: Array<Address>) {
        this._previousAddresses = value;
    }
}
