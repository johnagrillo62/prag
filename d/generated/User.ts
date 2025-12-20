export class User {
    private _name: string = '';
    private _age: number = 0;
    private _id: string = '';
    private _data: string = '';
    private _contact: ContactInfo;
    private _employer: Company;
    private _projects: Array<Project> = [];
    private _metadata: Map<string, string> = new Map();
    private _investments: Map<string, Company> = new Map();
    private _nested: Map<Map<string, string>, number> = new Map();

    constructor() {
        this._projects = [];
        this._metadata = new Map();
        this._investments = new Map();
        this._nested = new Map();
    }

    get name(): string {
        return this._name;
    }

    set name(value: string) {
        this._name = value;
    }

    get age(): number {
        return this._age;
    }

    set age(value: number) {
        this._age = value;
    }

    get id(): string {
        return this._id;
    }

    set id(value: string) {
        this._id = value;
    }

    get data(): string {
        return this._data;
    }

    set data(value: string) {
        this._data = value;
    }

    get contact(): ContactInfo {
        return this._contact;
    }

    set contact(value: ContactInfo) {
        this._contact = value;
    }

    get employer(): Company {
        return this._employer;
    }

    set employer(value: Company) {
        this._employer = value;
    }

    get projects(): Array<Project> {
        return this._projects;
    }

    set projects(value: Array<Project>) {
        this._projects = value;
    }

    get metadata(): Map<string, string> {
        return this._metadata;
    }

    set metadata(value: Map<string, string>) {
        this._metadata = value;
    }

    get investments(): Map<string, Company> {
        return this._investments;
    }

    set investments(value: Map<string, Company>) {
        this._investments = value;
    }

    get nested(): Map<Map<string, string>, number> {
        return this._nested;
    }

    set nested(value: Map<Map<string, string>, number>) {
        this._nested = value;
    }
}
