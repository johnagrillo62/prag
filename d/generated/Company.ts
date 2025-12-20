export class Company {
    private _name: string = '';
    private _headquarters: Address;
    private _taxId: string = '';
    private _offices: Map<string, Address> = new Map();

    constructor() {
        this._offices = new Map();
    }

    get name(): string {
        return this._name;
    }

    set name(value: string) {
        this._name = value;
    }

    get headquarters(): Address {
        return this._headquarters;
    }

    set headquarters(value: Address) {
        this._headquarters = value;
    }

    get taxId(): string {
        return this._taxId;
    }

    set taxId(value: string) {
        this._taxId = value;
    }

    get offices(): Map<string, Address> {
        return this._offices;
    }

    set offices(value: Map<string, Address>) {
        this._offices = value;
    }
}
